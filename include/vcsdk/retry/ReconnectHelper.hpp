#pragma once

#include <functional>
#include <thread>
#include <string>
#include "vcsdk/core/IConnectionPolicy.hpp"
#include "vcsdk/core/ConnectionState.hpp"
#include "vcsdk/core/ConnectorError.hpp"

namespace vcsdk::retry {

// Execute la sequence de reconnexion automatique (section 13), pilotee
// par une IConnectionPolicy (section 12). Reutilisable par N'IMPORTE
// QUEL connecteur (TCP, serie, CAN...), quel que soit son transport --
// c'est pour ca que 'tryConnect' est un callback generique plutot qu'un
// appel direct a une classe de transport particuliere.
//
// Suit exactement la machine d'etat de la section 6 :
//   CONNECTED -> DEGRADED -> RECONNECTING -> (CONNECTED | ERROR)
//
// Leve ConnectorError (CONNECTION_LOST) si toutes les tentatives echouent.
// Ne leve rien et retourne normalement si la reconnexion reussit.
inline void performReconnect(
    const std::function<void()>& tryConnect,
    const std::function<void(vcsdk::core::ConnectionState)>& setState,
    const vcsdk::core::IConnectionPolicy& policy,
    const std::string& connectorId,
    const std::string& timestamp) {
    using namespace vcsdk::core;

    setState(ConnectionState::DEGRADED);
    setState(ConnectionState::RECONNECTING);

    int attempt = 0;
    while (policy.shouldReconnect(attempt)) {
        attempt++;
        std::this_thread::sleep_for(policy.getNextRetryDelay(attempt));
        try {
            tryConnect();
            setState(ConnectionState::CONNECTED);
            return;  // succes : RECONNECTING -> CONNECTED
        } catch (const std::exception&) {
            // echec de cette tentative, on boucle (ou on sort si
            // shouldReconnect() devient false au tour suivant)
        }
    }

    setState(ConnectionState::ERROR);
    throw ConnectorError(
        ErrorCode::CONNECTION_LOST,
        "Reconnexion impossible apres " + std::to_string(attempt) + " tentative(s)",
        Severity::CRITICAL, connectorId, "", "", timestamp);
}

}  // namespace vcsdk::retry

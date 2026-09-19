#pragma once

#include "vcsdk/core/ConnectionState.hpp"
#include "vcsdk/core/RawVehicleMessage.hpp"
#include "vcsdk/core/Command.hpp"

namespace vcsdk::core {

// Interface centrale (section 21). Toute nouvelle famille de connecteur
// (Bluetooth, TCP, Serial, SocketCAN, Virtual...) DOIT heriter de cette
// classe et implementer ces 7 methodes -- rien de plus n'est autorise a
// etre suppose par le reste du SDK (CommandQueue, VehicleSession...).
//
// C'est cette interface qui rend possible le critere d'acceptation :
// "un nouveau connecteur peut etre ajoute sans modifier le Core" (section 33).
class IConnector {
public:
    virtual ~IConnector() = default;

    // Recherche les equipements/interfaces disponibles pour ce type de
    // transport (section 5). Peut ne rien faire pour un connecteur qui
    // n'a pas de notion de decouverte (ex: TCP avec host/port fixes).
    virtual void discover() = 0;

    // Etablit la connexion. Doit faire transiter l'etat via _setState()
    // dans l'implementation concrete : DISCONNECTED -> CONNECTING -> CONNECTED
    // (ou -> ERROR en cas d'echec, section 6).
    virtual void connect() = 0;

    // Ferme proprement la connexion. Doit amener l'etat a DISCONNECTED.
    virtual void disconnect() = 0;

    // Transmet une commande brute vers le vehicule. Le contenu de la
    // commande ne doit JAMAIS etre interprete ici (section 8).
    virtual void send(const Command& command) = 0;

    // Attend et retourne une reponse brute, sous forme standardisee
    // (section 9). Doit lever ConnectorError (TIMEOUT, READ_FAILED...)
    // en cas de probleme, jamais retourner un objet a moitie rempli.
    virtual RawVehicleMessage receive() = 0;

    // Verifie que la connexion est toujours vivante, sans attendre une
    // vraie commande metier (ping/health-check technique).
    virtual bool healthCheck() = 0;

    // Etat courant de la connexion (section 6).
    virtual ConnectionState getStatus() const = 0;
};

}  // namespace vcsdk::core

#pragma once

#include <string>
#include <chrono>
#include "vcsdk/core/IConnector.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"

namespace vcsdk::connectors {

using namespace vcsdk::core;

// Quatrieme connecteur du MVP (section 4, priorite 4) -- Bluetooth
// Classic via BlueZ (profil RFCOMM).
//
// *** AVERTISSEMENT HONNETE (a lire avant de presenter ce connecteur) ***
// Ce fichier est ecrit et compile correctement, mais N'A PAS PU ETRE
// TESTE FONCTIONNELLEMENT : l'environnement de developpement actuel ne
// dispose d'aucun controleur Bluetooth (meme socket(AF_BLUETOOTH, ...)
// echoue ici, faute de sous-systeme Bluetooth dans ce sandbox). La
// validation reelle necessite soit une machine avec un adaptateur
// Bluetooth physique, soit l'adaptateur ELM327 Bluetooth en attente de
// reception (voir feuille de route).
class BluetoothConnector : public IConnector {
public:
    // macAddress au format "AA:BB:CC:DD:EE:FF", channel RFCOMM (1 par defaut,
    // valeur la plus courante pour les adaptateurs ELM327 Bluetooth).
    explicit BluetoothConnector(std::string macAddress, int channel = 1,
                                 std::chrono::milliseconds readTimeout = std::chrono::milliseconds(1000));
    ~BluetoothConnector() override;

    // Recherche des peripheriques Bluetooth a proximite (section 5).
    // Non implemente au MVP : necessite un inquiry BlueZ complet
    // (hci_inquiry), laisse pour une iteration future -- voir TODO.
    void discover() override;

    void connect() override;
    void disconnect() override;
    void send(const Command& command) override;
    RawVehicleMessage receive() override;
    bool healthCheck() override;
    ConnectionState getStatus() const override;

    // Mode READ_ONLY actif par defaut (section 19, critere d'acceptation
    // section 33). A n'assouplir que de maniere explicite et deliberee.
    void setSecurityMode(vcsdk::security::SdkMode mode) { securityPolicy_.setMode(mode); }
    vcsdk::security::SdkMode getSecurityMode() const { return securityPolicy_.mode(); }

private:
    void setState(ConnectionState state);

    std::string macAddress_;
    int channel_;
    std::chrono::milliseconds readTimeout_;
    int socketFd_ = -1;
    ConnectionState state_ = ConnectionState::DISCONNECTED;
    vcsdk::security::CommandSecurityPolicy securityPolicy_;
    std::string recvBuffer_;
    int messageCounter_ = 0;
};

}  // namespace vcsdk::connectors

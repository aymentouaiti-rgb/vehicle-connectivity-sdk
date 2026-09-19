#pragma once

#include <string>
#include "vcsdk/core/IConnector.hpp"

namespace vcsdk::connectors {

using namespace vcsdk::core;

// Cinquieme et dernier connecteur du MVP (section 4, priorite 5).
// Cible une interface SocketCAN Linux native (can0 materielle, ou vcan0
// virtuelle pour les tests -- section 24).
//
// IMPORTANT -- convention de payload differente des autres connecteurs :
// les connecteurs texte (Virtual/Tcp/Serial) transportent des lignes
// ASCII de type "010C". Le bus CAN transporte des trames BINAIRES
// (identifiant + jusqu'a 8 octets de donnees). On adopte donc la
// notation standard de can-utils : "ID#DATA" en hexadecimal, ex:
//   "7DF#0201000000000000"
// C'est la meme convention que la commande "cansend" bien connue dans
// l'ecosysteme Linux automobile -- pas une invention arbitraire.
class SocketCanConnector : public IConnector {
public:
    explicit SocketCanConnector(std::string interfaceName);
    ~SocketCanConnector() override;

    // Verifie que l'interface (ex: "vcan0") existe dans le noyau, via
    // if_nametoindex() -- ne necessite pas de privilege particulier.
    void discover() override;

    void connect() override;
    void disconnect() override;
    void send(const Command& command) override;
    RawVehicleMessage receive() override;
    bool healthCheck() override;
    ConnectionState getStatus() const override;

    bool isInterfaceKnownToKernel() const { return interfaceFound_; }

private:
    void setState(ConnectionState state);

    std::string interfaceName_;
    int socketFd_ = -1;
    ConnectionState state_ = ConnectionState::DISCONNECTED;
    int messageCounter_ = 0;
    bool interfaceFound_ = false;
};

}  // namespace vcsdk::connectors

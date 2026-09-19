// Demonstrateur (section 32, livrable obligatoire) :
// ELM327 Emulator -> Vehicle Connectivity SDK -> lecture RPM/Speed/Temperature
// -> RawVehicleMessage
//
// Lancer d'abord le simulateur :
//   python3 tests/fixtures/elm327_emulator.py 35000
// Puis ce demonstrateur :
//   ./build/demo_virtual_connector

#include <iostream>
#include <memory>
#include "vcsdk/connectors/virtual/VirtualConnector.hpp"
#include "vcsdk/core/VehicleSession.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

int main() {
    std::cout << "=== Vehicle Connectivity SDK -- Demonstrateur ===\n\n";

    auto connector = std::make_shared<VirtualConnector>("127.0.0.1", 35000);
    VehicleSession session("SESSION-001", connector, "VIRTUAL");

    std::cout << "[1] Demarrage de la session (connect)...\n";
    session.start();
    std::cout << "    Etat : " << toString(session.getConnectionState()) << "\n\n";

    struct Param { const char* name; const char* pid; };
    Param params[] = {
        {"Regime moteur (RPM)", "010C"},
        {"Vitesse (Speed)", "010D"},
        {"Temperature moteur", "0105"},
    };

    for (const auto& p : params) {
        Command cmd;
        cmd.commandId = std::string("CMD-") + p.pid;
        cmd.payload = p.pid;
        cmd.priority = CommandPriority::Diagnostic;

        std::cout << "[2] Envoi commande " << p.name << " (" << p.pid << ")...\n";
        RawVehicleMessage msg = session.send(cmd);
        std::cout << "    -> RawVehicleMessage { messageId=" << msg.messageId
                  << ", rawPayload=\"" << msg.rawPayload << "\""
                  << ", status=" << msg.status
                  << ", latencyMs=" << msg.latencyMs << " }\n\n";
    }

    const auto& stats = session.getStatistics();
    std::cout << "[3] Statistiques session :\n"
              << "    commandsSent = " << stats.commandsSent << "\n"
              << "    messagesReceived = " << stats.messagesReceived << "\n\n";

    std::cout << "[4] Arret de la session (disconnect)...\n";
    session.stop();
    std::cout << "    Etat : " << toString(session.getConnectionState()) << "\n";

    std::cout << "\nDemonstration terminee avec succes.\n";
    return 0;
}

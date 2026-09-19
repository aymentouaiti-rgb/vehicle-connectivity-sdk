// Tests d'integration (section 32 : categorie de test distincte des tests
// contractuels). Un test contractuel valide UN connecteur isole contre
// le contrat IConnector. Un test d'integration valide que PLUSIEURS
// composants du SDK fonctionnent ENSEMBLE, de la configuration jusqu'a
// la reponse finale -- reproduit le scenario complet de la section 29.

#include <gtest/gtest.h>
#include <fstream>
#include "vcsdk/configuration/Config.hpp"
#include "vcsdk/factory/ConnectorFactory.hpp"
#include "vcsdk/core/VehicleSession.hpp"

using namespace vcsdk::core;
using namespace vcsdk::configuration;
using namespace vcsdk::factory;

namespace {
constexpr int kTestPort = 35030;

std::string writeConfig() {
    std::string path = "/tmp/vcsdk_integration_config.yaml";
    std::ofstream out(path);
    out << "connector:\n"
        << "  type: virtual\n"
        << "  host: 127.0.0.1\n"
        << "  port: " << kTestPort << "\n"
        << "security:\n"
        << "  mode: read_only\n";
    return path;
}
}  // namespace

// Reproduit exactement le scenario end-to-end de la section 29 :
// Config -> Factory -> VehicleSession -> send/receive -> RawVehicleMessage,
// exactement l'API interne proposee en section 31.
TEST(IntegrationTest, ChaineCompleteConfigFactorySessionReponse) {
    Config config = loadFromYamlFile(writeConfig());
    auto connector = ConnectorFactory::create(config);

    VehicleSession session("SESSION-INTEGRATION-1", connector, config.connectorType);
    session.start();
    EXPECT_EQ(session.getConnectionState(), ConnectionState::CONNECTED);

    Command cmd;
    cmd.commandId = "CMD-1";
    cmd.payload = "010C";
    RawVehicleMessage msg = session.send(cmd);

    EXPECT_EQ(msg.rawPayload, "410C1AF8");
    EXPECT_EQ(msg.sessionId, "SESSION-INTEGRATION-1");
    EXPECT_EQ(session.getStatistics().commandsSent, 1);
    EXPECT_EQ(session.getStatistics().messagesReceived, 1);

    session.stop();
    EXPECT_EQ(session.getConnectionState(), ConnectionState::DISCONNECTED);
}

// Integration : la securite (mode configure via YAML) doit se propager
// jusqu'au connecteur reellement instancie par la Factory.
TEST(IntegrationTest, ModeSecuriteConfigureViaYamlEstApplique) {
    Config config = loadFromYamlFile(writeConfig());  // security.mode: read_only
    auto connector = ConnectorFactory::create(config);
    connector->connect();

    Command clearDtc;
    clearDtc.commandId = "CMD-CLEAR";
    clearDtc.payload = "04";
    EXPECT_THROW(connector->send(clearDtc), ConnectorError);

    connector->disconnect();
}

// Integration : plusieurs sessions successives sur le meme connecteur
// (creation, utilisation, arret) ne doivent pas se corrompre entre elles.
TEST(IntegrationTest, SessionsSuccessivesRestentIndependantes) {
    Config config = loadFromYamlFile(writeConfig());

    for (int i = 0; i < 3; ++i) {
        auto connector = ConnectorFactory::create(config);
        VehicleSession session("SESSION-" + std::to_string(i), connector, config.connectorType);
        session.start();

        Command cmd;
        cmd.commandId = "CMD-" + std::to_string(i);
        cmd.payload = "010D";
        RawVehicleMessage msg = session.send(cmd);
        EXPECT_EQ(msg.rawPayload, "410D48");
        EXPECT_EQ(msg.sessionId, "SESSION-" + std::to_string(i));

        session.stop();
    }
}

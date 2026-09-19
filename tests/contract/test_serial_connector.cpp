// Tests contractuels du SerialConnector. Necessite une paire de
// pseudo-terminaux (voir tests/run_tests.sh) : le connecteur se connecte
// sur un bout, l'emulateur ecoute sur l'autre.

#include <gtest/gtest.h>
#include <cstdlib>
#include "vcsdk/connectors/serial/SerialConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

namespace {
std::string clientDevicePath() {
    const char* env = std::getenv("VCSDK_TEST_SERIAL_CLIENT_PATH");
    return env ? env : "/tmp/vcsdk_ttyA";
}
}

TEST(SerialConnectorContract, DiscoverTrouveLePeripheriqueQuandPresent) {
    SerialConnector connector(clientDevicePath());
    connector.discover();
    // Pas d'API publique dediee ici (contrairement a TcpConnector) --
    // on verifie indirectement via connect(), qui doit reussir.
    EXPECT_NO_THROW(connector.connect());
    connector.disconnect();
}

TEST(SerialConnectorContract, PeripheriqueAbsent) {
    SerialConnector connector("/dev/ttyUSB_INEXISTANT_999");
    EXPECT_THROW(connector.connect(), ConnectorError);
    EXPECT_EQ(connector.getStatus(), ConnectionState::ERROR);
}

TEST(SerialConnectorContract, ConnexionEtEchangeNormal) {
    SerialConnector connector(clientDevicePath());
    connector.connect();
    EXPECT_EQ(connector.getStatus(), ConnectionState::CONNECTED);

    Command cmd;
    cmd.commandId = "CMD-1";
    cmd.payload = "010C";
    connector.send(cmd);

    RawVehicleMessage msg = connector.receive();
    EXPECT_EQ(msg.rawPayload, "410C1AF8");
    EXPECT_EQ(msg.status, "SUCCESS");

    connector.disconnect();
    EXPECT_EQ(connector.getStatus(), ConnectionState::DISCONNECTED);
}

TEST(SerialConnectorContract, HealthCheckRefleteEtat) {
    SerialConnector connector(clientDevicePath());
    EXPECT_FALSE(connector.healthCheck());
    connector.connect();
    EXPECT_TRUE(connector.healthCheck());
    connector.disconnect();
    EXPECT_FALSE(connector.healthCheck());
}

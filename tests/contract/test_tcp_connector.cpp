// Tests contractuels du TcpConnector -- memes scenarios que VirtualConnector
// (section 25), plus un test specifique sur discover() qui lui est propre.

#include <gtest/gtest.h>
#include "vcsdk/connectors/tcp/TcpConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

namespace {
constexpr int kTestPort = 35011;
}

TEST(TcpConnectorContract, DiscoverTrouveUneInterfaceQuandServeurPresent) {
    TcpConnector connector("127.0.0.1", kTestPort);
    connector.discover();
    ASSERT_EQ(connector.getDiscoveredInterfaces().size(), 1u);
    EXPECT_EQ(connector.getDiscoveredInterfaces()[0], "127.0.0.1:35011");
}

TEST(TcpConnectorContract, DiscoverNeTrouveRienSiAucunServeur) {
    TcpConnector connector("127.0.0.1", /*port ferme*/ 2);
    connector.discover();
    EXPECT_TRUE(connector.getDiscoveredInterfaces().empty());
}

TEST(TcpConnectorContract, ConnexionNormale) {
    TcpConnector connector("127.0.0.1", kTestPort);
    connector.connect();
    EXPECT_EQ(connector.getStatus(), ConnectionState::CONNECTED);

    Command cmd;
    cmd.commandId = "CMD-1";
    cmd.payload = "010C";
    connector.send(cmd);

    RawVehicleMessage msg = connector.receive();
    EXPECT_EQ(msg.rawPayload, "410C1AF8");

    connector.disconnect();
    EXPECT_EQ(connector.getStatus(), ConnectionState::DISCONNECTED);
}

TEST(TcpConnectorContract, AdaptateurAbsent) {
    TcpConnector connector("127.0.0.1", /*port inexistant*/ 2);
    EXPECT_THROW(connector.connect(), ConnectorError);
    EXPECT_EQ(connector.getStatus(), ConnectionState::ERROR);
}

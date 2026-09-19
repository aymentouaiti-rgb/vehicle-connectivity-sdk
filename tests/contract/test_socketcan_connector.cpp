// Tests contractuels du SocketCanConnector.
// IMPORTANT : ce sandbox ne dispose pas du module noyau vcan (interface
// CAN virtuelle) -- ces tests couvrent donc uniquement ce qui est
// verifiable SANS interface CAN reelle : la decouverte (absence) et la
// gestion d'erreur a la connexion. Le scenario "connexion normale"
// devra etre revalide sur une machine disposant de vcan0 ou d'un vrai
// bus CAN (voir documentation).

#include <gtest/gtest.h>
#include "vcsdk/connectors/socketcan/SocketCanConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

TEST(SocketCanConnectorContract, DiscoverNeTrouveRienSansInterfaceVcan) {
    SocketCanConnector connector("vcan_absent_999");
    connector.discover();
    EXPECT_FALSE(connector.isInterfaceKnownToKernel());
}

TEST(SocketCanConnectorContract, ConnectEchoueProprementSansInterface) {
    SocketCanConnector connector("vcan_absent_999");
    EXPECT_THROW(connector.connect(), ConnectorError);
    EXPECT_EQ(connector.getStatus(), ConnectionState::ERROR);
}

TEST(SocketCanConnectorContract, HealthCheckFalseAvantConnexion) {
    SocketCanConnector connector("vcan_absent_999");
    EXPECT_FALSE(connector.healthCheck());
}

// Tests du BluetoothConnector.
// AVERTISSEMENT : ce sandbox ne dispose d'AUCUN sous-systeme Bluetooth
// (socket(AF_BLUETOOTH, ...) echoue au niveau du noyau lui-meme). Seul
// le test ci-dessous est honnetement verifiable ici : que l'echec de
// creation de socket est bien remonte comme une ConnectorError propre,
// sans crash. Tout le reste (connexion RFCOMM reelle, echange de
// donnees) doit etre revalide sur une machine avec adaptateur Bluetooth.

#include <gtest/gtest.h>
#include "vcsdk/connectors/bluetooth/BluetoothConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

TEST(BluetoothConnectorContract, ConnectSansAdaptateurLeveConnectorErrorProprement) {
    BluetoothConnector connector("AA:BB:CC:DD:EE:FF");
    EXPECT_THROW(connector.connect(), ConnectorError);
    EXPECT_EQ(connector.getStatus(), ConnectionState::ERROR);
}

TEST(BluetoothConnectorContract, HealthCheckFalseAvantConnexion) {
    BluetoothConnector connector("AA:BB:CC:DD:EE:FF");
    EXPECT_FALSE(connector.healthCheck());
}

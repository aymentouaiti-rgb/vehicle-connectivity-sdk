#include <gtest/gtest.h>
#include <fstream>
#include "vcsdk/configuration/Config.hpp"
#include "vcsdk/factory/ConnectorFactory.hpp"

using namespace vcsdk::configuration;
using namespace vcsdk::factory;

namespace {
// Reproduit exactement l'exemple YAML de la section 30 de la specification.
const char* kExampleYaml = R"(
connector:
  type: tcp
  host: 127.0.0.1
  port: 35000
timeouts:
  connect: 5
  read: 2
  command: 2
retry:
  enabled: true
  max_attempts: 3
  strategy: exponential
security:
  mode: read_only
logging:
  level: INFO
)";

std::string writeTempYaml() {
    std::string path = "/tmp/vcsdk_test_config.yaml";
    std::ofstream out(path);
    out << kExampleYaml;
    return path;
}
}  // namespace

TEST(Config, ChargeExactementLExempleDeLaSpec) {
    Config config = loadFromYamlFile(writeTempYaml());

    EXPECT_EQ(config.connectorType, "tcp");
    EXPECT_EQ(config.host, "127.0.0.1");
    EXPECT_EQ(config.port, 35000);
    EXPECT_EQ(config.connectTimeout.count(), 5);
    EXPECT_EQ(config.readTimeout.count(), 2);
    EXPECT_TRUE(config.retryEnabled);
    EXPECT_EQ(config.retryMaxAttempts, 3);
    EXPECT_EQ(config.retryStrategy, "exponential");
    EXPECT_EQ(config.securityMode, "read_only");
    EXPECT_EQ(config.loggingLevel, "INFO");
}

TEST(Config, ValeursParDefautSiFichierPartiel) {
    std::string path = "/tmp/vcsdk_test_config_partial.yaml";
    std::ofstream(path) << "connector:\n  type: virtual\n";
    Config config = loadFromYamlFile(path);

    EXPECT_EQ(config.connectorType, "virtual");
    EXPECT_EQ(config.retryMaxAttempts, 3);  // valeur par defaut, non ecrasee
}

TEST(Config, FichierInexistantLeveUneErreur) {
    EXPECT_THROW(loadFromYamlFile("/tmp/vcsdk_ce_fichier_nexiste_pas.yaml"), std::runtime_error);
}

// Section 31 : "connector = ConnectorFactory.create(config)"
TEST(ConnectorFactory, CreeLeBonTypeDeConnecteur) {
    Config config;
    config.connectorType = "virtual";
    config.host = "127.0.0.1";
    config.port = 35000;

    auto connector = ConnectorFactory::create(config);
    ASSERT_NE(connector, nullptr);
    EXPECT_EQ(connector->getStatus(), vcsdk::core::ConnectionState::DISCONNECTED);
}

TEST(ConnectorFactory, TypeInconnuLeveUneErreur) {
    Config config;
    config.connectorType = "hologramme";
    EXPECT_THROW(ConnectorFactory::create(config), std::runtime_error);
}

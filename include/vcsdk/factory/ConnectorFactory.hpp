#pragma once

#include <memory>
#include <stdexcept>
#include "vcsdk/core/IConnector.hpp"
#include "vcsdk/configuration/Config.hpp"
#include "vcsdk/connectors/virtual/VirtualConnector.hpp"
#include "vcsdk/connectors/tcp/TcpConnector.hpp"
#include "vcsdk/connectors/serial/SerialConnector.hpp"
#include "vcsdk/connectors/bluetooth/BluetoothConnector.hpp"
#include "vcsdk/connectors/socketcan/SocketCanConnector.hpp"

namespace vcsdk::factory {

// Pattern Factory impose par la section 22. Correspond exactement a
// l'API interne proposee en section 31 :
//   connector = ConnectorFactory.create(config)
// Le code appelant ne connait jamais la classe concrete instanciee --
// c'est precisement ce qui permet d'ajouter un futur connecteur sans
// toucher au reste de la plateforme (critere d'acceptation section 33).
class ConnectorFactory {
public:
    static std::shared_ptr<vcsdk::core::IConnector> create(const vcsdk::configuration::Config& config) {
        using namespace vcsdk::connectors;

        if (config.connectorType == "virtual") {
            return std::make_shared<VirtualConnector>(
                config.host, config.port, config.connectTimeout, config.readTimeout);
        }
        if (config.connectorType == "tcp") {
            return std::make_shared<TcpConnector>(
                config.host, config.port, config.connectTimeout, config.readTimeout);
        }
        if (config.connectorType == "serial") {
            return std::make_shared<SerialConnector>(config.devicePath, 38400, config.readTimeout);
        }
        if (config.connectorType == "bluetooth") {
            return std::make_shared<BluetoothConnector>(config.macAddress, 1, config.readTimeout);
        }
        if (config.connectorType == "socketcan") {
            return std::make_shared<SocketCanConnector>(config.interfaceName);
        }

        throw std::runtime_error("UNSUPPORTED_CONNECTOR: " + config.connectorType);
    }
};

}  // namespace vcsdk::factory

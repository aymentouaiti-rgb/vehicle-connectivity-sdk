#include "vcsdk/configuration/Config.hpp"

#include <yaml-cpp/yaml.h>
#include <stdexcept>

namespace vcsdk::configuration {

Config loadFromYamlFile(const std::string& path) {
    Config config;  // valeurs par defaut

    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    } catch (const std::exception& e) {
        throw std::runtime_error("impossible de charger la configuration: " + std::string(e.what()));
    }

    if (auto n = root["connector"]) {
        if (n["type"]) config.connectorType = n["type"].as<std::string>();
        if (n["host"]) config.host = n["host"].as<std::string>();
        if (n["port"]) config.port = n["port"].as<int>();
        if (n["device_path"]) config.devicePath = n["device_path"].as<std::string>();
        if (n["mac_address"]) config.macAddress = n["mac_address"].as<std::string>();
        if (n["interface"]) config.interfaceName = n["interface"].as<std::string>();
    }

    if (auto n = root["timeouts"]) {
        if (n["connect"]) config.connectTimeout = std::chrono::seconds(n["connect"].as<int>());
        if (n["read"]) config.readTimeout = std::chrono::seconds(n["read"].as<int>());
        if (n["command"]) config.commandTimeout = std::chrono::seconds(n["command"].as<int>());
    }

    if (auto n = root["retry"]) {
        if (n["enabled"]) config.retryEnabled = n["enabled"].as<bool>();
        if (n["max_attempts"]) config.retryMaxAttempts = n["max_attempts"].as<int>();
        if (n["strategy"]) config.retryStrategy = n["strategy"].as<std::string>();
    }

    if (auto n = root["security"]) {
        if (n["mode"]) config.securityMode = n["mode"].as<std::string>();
    }

    if (auto n = root["logging"]) {
        if (n["level"]) config.loggingLevel = n["level"].as<std::string>();
    }

    if (auto n = root["rate_limit"]) {
        if (n["max_commands_per_second"])
            config.maxCommandRatePerSecond = n["max_commands_per_second"].as<int>();
        if (n["max_queue_size"])
            config.maxQueueSize = n["max_queue_size"].as<size_t>();
    }

    return config;
}

}  // namespace vcsdk::configuration

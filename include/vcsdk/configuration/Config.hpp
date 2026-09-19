#pragma once

#include <string>
#include <chrono>

namespace vcsdk::configuration {

// Reflete exactement l'exemple YAML de la section 30 :
//   connector: {type, host, port}
//   timeouts: {connect, read, command}
//   retry: {enabled, max_attempts, strategy}
//   security: {mode}
//   logging: {level}
struct Config {
    std::string connectorType = "virtual";  // "virtual" | "tcp" | "serial" | "bluetooth" | "socketcan"
    std::string host = "127.0.0.1";
    int port = 35000;
    std::string devicePath = "/dev/ttyUSB0";       // pour "serial"
    std::string macAddress = "00:00:00:00:00:00";  // pour "bluetooth"
    std::string interfaceName = "vcan0";           // pour "socketcan"

    std::chrono::seconds connectTimeout{5};
    std::chrono::seconds readTimeout{2};
    std::chrono::seconds commandTimeout{2};

    bool retryEnabled = true;
    int retryMaxAttempts = 3;
    std::string retryStrategy = "exponential";  // "fixed" | "exponential"

    std::string securityMode = "read_only";  // "read_only" | "full"
    std::string loggingLevel = "INFO";

    int maxCommandRatePerSecond = 20;
    size_t maxQueueSize = 1000;
};

// Charge une configuration depuis un fichier YAML (section 30). Les
// champs absents du fichier gardent leur valeur par defaut ci-dessus --
// aucune valeur n'est jamais codee en dur dans le code appelant.
Config loadFromYamlFile(const std::string& path);

}  // namespace vcsdk::configuration

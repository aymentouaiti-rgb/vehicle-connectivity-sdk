#pragma once

#include <string>
#include <chrono>
#include "vcsdk/core/IConnector.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/core/IConnectionPolicy.hpp"
#include "vcsdk/net/SerialPortClient.hpp"
#include "vcsdk/retry/RetryPolicy.hpp"

namespace vcsdk::connectors {

using namespace vcsdk::core;

// Troisieme connecteur du MVP (section 4, priorite 3). Cible un
// adaptateur ELM327 branche en USB Serial ou en Bluetooth Classic (le
// profil SPP expose aussi un port serie une fois l'appareil jumele --
// meme mecanique cote SDK).
class SerialConnector : public IConnector {
public:
    SerialConnector(std::string devicePath, int baudRate = 38400,
                     std::chrono::milliseconds readTimeout = std::chrono::milliseconds(1000));
    ~SerialConnector() override;

    // Verifie que le chemin (ex: /dev/ttyUSB0) existe et est accessible,
    // sans ouvrir de session complete (section 5).
    void discover() override;

    void connect() override;
    void disconnect() override;
    void send(const Command& command) override;
    RawVehicleMessage receive() override;
    bool healthCheck() override;
    ConnectionState getStatus() const override;

    // Mode READ_ONLY actif par defaut (section 19, critere d'acceptation
    // section 33). A n'assouplir que de maniere explicite et deliberee.
    void setSecurityMode(vcsdk::security::SdkMode mode) { securityPolicy_.setMode(mode); }
    vcsdk::security::SdkMode getSecurityMode() const { return securityPolicy_.mode(); }

private:
    void setState(ConnectionState state);

    std::string devicePath_;
    int baudRate_;
    vcsdk::net::SerialPortClient port_;
    vcsdk::retry::RetryPolicy retryPolicy_;
    ConnectionState state_ = ConnectionState::DISCONNECTED;
    vcsdk::security::CommandSecurityPolicy securityPolicy_;
    int messageCounter_ = 0;
    bool discovered_ = false;
};

}  // namespace vcsdk::connectors

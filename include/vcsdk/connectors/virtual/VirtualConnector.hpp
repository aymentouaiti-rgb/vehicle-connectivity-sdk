#pragma once

#include <string>
#include <chrono>
#include "vcsdk/core/IConnector.hpp"
#include "vcsdk/core/IConnectionPolicy.hpp"
#include "vcsdk/net/TcpSocketClient.hpp"
#include "vcsdk/retry/RetryPolicy.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/monitoring/Metrics.hpp"
#include "vcsdk/monitoring/RateLimiter.hpp"
#include "vcsdk/logging/StructuredLogger.hpp"
#include "vcsdk/buffer/LocalBuffer.hpp"

namespace vcsdk::connectors {

using namespace vcsdk::core;

// Premier connecteur a developper selon la specification (section 24).
// Permet de developper et tester sans vehicule reel, en se connectant a
// un simulateur TCP (ex: un ELM327 Emulator qui ecoute sur host:port).
//
// Utilise TcpSocketClient par COMPOSITION (pattern Adapter, section 22) :
// VirtualConnector ne sait rien de la mecanique socket, il se contente
// de l'orchestrer et de la traduire dans le vocabulaire du SDK
// (ConnectionState, ConnectorError, RawVehicleMessage).
//
// C'est le connecteur "de reference" : il integre TOUTES les briques du
// MVP (fiabilite, securite, buffer, debit, logs, metriques) -- les
// autres connecteurs suivent le meme schema d'integration.
class VirtualConnector : public IConnector {
public:
    VirtualConnector(std::string host, int port,
                      std::chrono::milliseconds connectTimeout = std::chrono::milliseconds(5000),
                      std::chrono::milliseconds readTimeout = std::chrono::milliseconds(1000));
    ~VirtualConnector() override;

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

    // Metriques techniques (section 18) -- lecture seule pour l'appelant.
    const vcsdk::monitoring::Metrics& getMetrics() const { return metrics_; }

    // Derniers messages recus, conserves temporairement (section 14).
    vcsdk::buffer::LocalBuffer<RawVehicleMessage>& getBuffer() { return buffer_; }

    // Permet de configurer le debit maximal (section 15) et le logger
    // (section 17) depuis l'exterieur (ex: via Config, section 30).
    void setMaxCommandRate(int perSecond) { rateLimiter_.setMaxRequestsPerSecond(perSecond); }
    void setLogLevel(vcsdk::logging::LogLevel level) { logger_.setMinLevel(level); }

private:
    void setState(ConnectionState state);

    std::string host_;
    int port_;
    std::chrono::milliseconds connectTimeout_;
    vcsdk::net::TcpSocketClient socket_;
    vcsdk::retry::RetryPolicy retryPolicy_;
    vcsdk::security::CommandSecurityPolicy securityPolicy_;
    vcsdk::monitoring::Metrics metrics_;
    vcsdk::monitoring::RateLimiter rateLimiter_;
    vcsdk::logging::StructuredLogger logger_;
    vcsdk::buffer::LocalBuffer<RawVehicleMessage> buffer_{100};
    ConnectionState state_ = ConnectionState::DISCONNECTED;
    int messageCounter_ = 0;
    std::string sessionId_ = "SESSION-VIRTUAL";
};

}  // namespace vcsdk::connectors

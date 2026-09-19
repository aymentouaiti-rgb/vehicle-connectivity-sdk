#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <ctime>
#include "vcsdk/core/IConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"

namespace vcsdk::core {

// Statistiques techniques exposees par une session (alimentent les
// metriques de la section 18).
struct SessionStatistics {
    int commandsSent = 0;
    int messagesReceived = 0;
    int timeouts = 0;
    int errors = 0;
    double averageLatencyMs = 0.0;
};

// Encapsule le cycle de vie complet d'une connexion (section 7).
// C'est l'objet que le reste de la plateforme (Protocol Engine) manipule --
// jamais directement un IConnector.
class VehicleSession {
public:
    VehicleSession(std::string sessionId,
                    std::shared_ptr<IConnector> connector,
                    std::string connectorType)
        : sessionId_(std::move(sessionId)),
          connector_(std::move(connector)),
          connectorType_(std::move(connectorType)) {}

    // Demarre la session : connecte le connecteur sous-jacent et
    // enregistre le timestamp de debut (section 7).
    void start() {
        startTimestamp_ = nowIso8601();
        connector_->connect();
    }

    // Arrete proprement la session.
    void stop() {
        connector_->disconnect();
        endTimestamp_ = nowIso8601();
    }

    // Envoie une commande via le connecteur sous-jacent et met a jour
    // les statistiques (commandsSent).
    RawVehicleMessage send(const Command& command) {
        connector_->send(command);
        stats_.commandsSent++;
        RawVehicleMessage msg = connector_->receive();
        stats_.messagesReceived++;
        msg.sessionId = sessionId_;
        return msg;
    }

    const SessionStatistics& getStatistics() const { return stats_; }
    ConnectionState getConnectionState() const { return connector_->getStatus(); }

    const std::string& sessionId() const { return sessionId_; }
    const std::string& connectorType() const { return connectorType_; }
    const std::string& startTimestamp() const { return startTimestamp_; }
    const std::string& endTimestamp() const { return endTimestamp_; }

private:
    static std::string nowIso8601() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
        return std::string(buf);
    }

    std::string sessionId_;
    std::shared_ptr<IConnector> connector_;
    std::string connectorType_;
    std::string startTimestamp_;
    std::string endTimestamp_;
    SessionStatistics stats_;
};

}  // namespace vcsdk::core

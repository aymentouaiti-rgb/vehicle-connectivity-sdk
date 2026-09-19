#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "vcsdk/core/IConnector.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/core/IConnectionPolicy.hpp"
#include "vcsdk/net/TcpSocketClient.hpp"
#include "vcsdk/retry/RetryPolicy.hpp"

namespace vcsdk::connectors {

using namespace vcsdk::core;

// Deuxieme connecteur du MVP (section 4, priorite 2). Contrairement au
// VirtualConnector (qui cible un simulateur de developpement local),
// TcpConnector cible un VRAI adaptateur reseau -- typiquement un boitier
// OBD-II WiFi qui expose un serveur TCP (convention tres repandue chez
// les adaptateurs WiFi grand public, souvent sur 192.168.0.10:35000).
//
// Techniquement, le transport est identique a VirtualConnector (meme
// TcpSocketClient) -- ce qui differe, c'est le ROLE dans l'architecture
// et la strategie de decouverte (discover() essaie des adresses reseau
// reelles, pas un simulateur fixe).
class TcpConnector : public IConnector {
public:
    TcpConnector(std::string host, int port,
                 std::chrono::milliseconds connectTimeout = std::chrono::milliseconds(5000),
                 std::chrono::milliseconds readTimeout = std::chrono::milliseconds(1000));
    ~TcpConnector() override;

    // Tente de confirmer que l'hote:port configure est bien joignable,
    // sans etablir de connexion complete (juste un test de portee).
    // Alimente la liste retournee par discover() (section 5).
    void discover() override;

    void connect() override;
    void disconnect() override;
    void send(const Command& command) override;
    RawVehicleMessage receive() override;
    bool healthCheck() override;
    ConnectionState getStatus() const override;

    // Interfaces reseau trouvees lors du dernier discover() -- expose en
    // plus du contrat IConnector, pour inspection (section 5 : format
    // "Network: 127.0.0.1:35000").
    const std::vector<std::string>& getDiscoveredInterfaces() const { return discovered_; }

    // Mode READ_ONLY actif par defaut (section 19, critere d'acceptation
    // section 33). A n'assouplir que de maniere explicite et deliberee.
    void setSecurityMode(vcsdk::security::SdkMode mode) { securityPolicy_.setMode(mode); }
    vcsdk::security::SdkMode getSecurityMode() const { return securityPolicy_.mode(); }

private:
    void setState(ConnectionState state);

    std::string host_;
    int port_;
    std::chrono::milliseconds connectTimeout_;
    vcsdk::net::TcpSocketClient socket_;
    vcsdk::retry::RetryPolicy retryPolicy_;
    ConnectionState state_ = ConnectionState::DISCONNECTED;
    vcsdk::security::CommandSecurityPolicy securityPolicy_;
    int messageCounter_ = 0;
    std::vector<std::string> discovered_;
};

}  // namespace vcsdk::connectors

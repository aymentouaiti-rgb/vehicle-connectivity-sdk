#include "vcsdk/connectors/tcp/TcpConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/retry/ReconnectHelper.hpp"

#include <chrono>
#include <ctime>

namespace vcsdk::connectors {

namespace {
std::string nowIso8601() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return std::string(buf);
}
}  // namespace

TcpConnector::TcpConnector(std::string host, int port,
                            std::chrono::milliseconds connectTimeout,
                            std::chrono::milliseconds readTimeout)
    : host_(std::move(host)), port_(port),
      connectTimeout_(connectTimeout), socket_(readTimeout) {}

TcpConnector::~TcpConnector() {
    socket_.close();
}

void TcpConnector::setState(ConnectionState state) {
    state_ = state;
}

void TcpConnector::discover() {
    setState(ConnectionState::DISCOVERING);
    discovered_.clear();

    // Verification "portee" : on ouvre puis referme immediatement une
    // connexion, sans passer par connect() (qui, lui, doit rester l'etat
    // CONNECTED apres coup). C'est une simplification du MVP -- une vraie
    // decouverte irait plus loin (scan mDNS/SSDP des boitiers WiFi OBD-II
    // connus, essai de plusieurs adresses par defaut, etc.)
    vcsdk::net::TcpSocketClient probe(std::chrono::milliseconds(500));
    try {
        probe.open(host_, port_, connectTimeout_);
        discovered_.push_back(host_ + ":" + std::to_string(port_));
        probe.close();
    } catch (const std::exception&) {
        // Rien trouve : discovered_ reste vide, ce n'est pas une erreur
        // fatale (section 25 : "Discover -> No device" est un scenario
        // normal, l'appelant decide quoi faire ensuite).
    }

    setState(ConnectionState::DISCONNECTED);
}

void TcpConnector::connect() {
    setState(ConnectionState::CONNECTING);
    try {
        socket_.open(host_, port_, connectTimeout_);
    } catch (const std::exception& e) {
        setState(ConnectionState::ERROR);
        throw ConnectorError(ErrorCode::CONNECTOR_NOT_FOUND, e.what(),
            Severity::ERROR, "TcpConnector", "", "", nowIso8601());
    }
    setState(ConnectionState::CONNECTED);
}

void TcpConnector::disconnect() {
    setState(ConnectionState::DISCONNECTING);
    socket_.close();
    setState(ConnectionState::DISCONNECTED);
}

void TcpConnector::send(const Command& command) {
    if (state_ != ConnectionState::CONNECTED) {
        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "send() appele hors connexion", Severity::ERROR,
            "TcpConnector", "", command.correlationId, nowIso8601());
    }
    // Validation de securite AVANT toute emission (section 19).
    securityPolicy_.validate(command);
    try {
        socket_.writeLine(command.payload);
    } catch (const std::exception& e) {
        setState(ConnectionState::DEGRADED);
        throw ConnectorError(ErrorCode::WRITE_FAILED, e.what(),
            Severity::ERROR, "TcpConnector", "", command.correlationId, nowIso8601());
    }
}

RawVehicleMessage TcpConnector::receive() {
    auto start = std::chrono::steady_clock::now();
    std::string payload;
    try {
        payload = socket_.readUntilPrompt();
    } catch (const std::exception& e) {
        std::string what = e.what();
        if (what == "TIMEOUT") {
            throw ConnectorError(ErrorCode::TIMEOUT,
                "Aucune reponse recue dans le delai imparti", Severity::WARNING,
                "TcpConnector", "", "", nowIso8601());
        }

        vcsdk::retry::performReconnect(
            [this]() { socket_.close(); socket_.open(host_, port_, connectTimeout_); },
            [this](ConnectionState s) { setState(s); },
            retryPolicy_, "TcpConnector", nowIso8601());

        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "Lien perdu ; reconnexion automatique reussie -- veuillez renvoyer votre commande",
            Severity::WARNING, "TcpConnector", "", "", nowIso8601());
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    RawVehicleMessage msg;
    msg.messageId = "MSG-" + std::to_string(++messageCounter_);
    msg.timestamp = nowIso8601();
    msg.protocol = "OBD2";
    msg.source = "ECU";
    msg.rawPayload = payload;
    msg.status = "SUCCESS";
    msg.latencyMs = static_cast<int>(elapsed.count());
    return msg;
}

bool TcpConnector::healthCheck() {
    return state_ == ConnectionState::CONNECTED && socket_.isOpen();
}

ConnectionState TcpConnector::getStatus() const {
    return state_;
}

}  // namespace vcsdk::connectors

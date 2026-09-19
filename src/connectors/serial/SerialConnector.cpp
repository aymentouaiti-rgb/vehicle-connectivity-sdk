#include "vcsdk/connectors/serial/SerialConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/retry/ReconnectHelper.hpp"

#include <sys/stat.h>
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

SerialConnector::SerialConnector(std::string devicePath, int baudRate,
                                  std::chrono::milliseconds readTimeout)
    : devicePath_(std::move(devicePath)), baudRate_(baudRate), port_(readTimeout) {}

SerialConnector::~SerialConnector() {
    port_.close();
}

void SerialConnector::setState(ConnectionState state) {
    state_ = state;
}

void SerialConnector::discover() {
    setState(ConnectionState::DISCOVERING);
    struct stat st{};
    discovered_ = (::stat(devicePath_.c_str(), &st) == 0);
    setState(ConnectionState::DISCONNECTED);
}

void SerialConnector::connect() {
    setState(ConnectionState::CONNECTING);
    try {
        port_.open(devicePath_, baudRate_);
    } catch (const std::exception& e) {
        setState(ConnectionState::ERROR);
        throw ConnectorError(ErrorCode::CONNECTOR_NOT_FOUND, e.what(),
            Severity::ERROR, "SerialConnector", "", "", nowIso8601());
    }
    setState(ConnectionState::CONNECTED);
}

void SerialConnector::disconnect() {
    setState(ConnectionState::DISCONNECTING);
    port_.close();
    setState(ConnectionState::DISCONNECTED);
}

void SerialConnector::send(const Command& command) {
    if (state_ != ConnectionState::CONNECTED) {
        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "send() appele hors connexion", Severity::ERROR,
            "SerialConnector", "", command.correlationId, nowIso8601());
    }
    // Validation de securite AVANT toute emission (section 19).
    securityPolicy_.validate(command);
    try {
        port_.writeLine(command.payload);
    } catch (const std::exception& e) {
        setState(ConnectionState::DEGRADED);
        throw ConnectorError(ErrorCode::WRITE_FAILED, e.what(),
            Severity::ERROR, "SerialConnector", "", command.correlationId, nowIso8601());
    }
}

RawVehicleMessage SerialConnector::receive() {
    auto start = std::chrono::steady_clock::now();
    std::string payload;
    try {
        payload = port_.readUntilPrompt();
    } catch (const std::exception& e) {
        std::string what = e.what();
        if (what == "TIMEOUT") {
            throw ConnectorError(ErrorCode::TIMEOUT,
                "Aucune reponse recue dans le delai imparti", Severity::WARNING,
                "SerialConnector", "", "", nowIso8601());
        }

        // Contrairement a TCP, un port serie n'a pas de "fermeture propre
        // par le pair" -- une erreur de lecture ici signifie generalement
        // un peripherique debranche ou en defaut. On tente quand meme une
        // reconnexion automatique (section 13) : si le peripherique
        // reapparait (ex: reconnexion USB), ca fonctionnera.
        vcsdk::retry::performReconnect(
            [this]() { port_.close(); port_.open(devicePath_, baudRate_); },
            [this](ConnectionState s) { setState(s); },
            retryPolicy_, "SerialConnector", nowIso8601());

        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "Lien perdu ; reconnexion automatique reussie -- veuillez renvoyer votre commande",
            Severity::WARNING, "SerialConnector", "", "", nowIso8601());
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

bool SerialConnector::healthCheck() {
    return state_ == ConnectionState::CONNECTED && port_.isOpen();
}

ConnectionState SerialConnector::getStatus() const {
    return state_;
}

}  // namespace vcsdk::connectors

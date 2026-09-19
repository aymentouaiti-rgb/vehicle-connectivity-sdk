#include "vcsdk/connectors/virtual/VirtualConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"
#include "vcsdk/retry/ReconnectHelper.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"

#include <chrono>
#include <ctime>

namespace vcsdk::connectors {

using vcsdk::logging::LogLevel;
using vcsdk::logging::LogField;

namespace {
std::string nowIso8601() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return std::string(buf);
}
}  // namespace

VirtualConnector::VirtualConnector(std::string host, int port,
                                    std::chrono::milliseconds connectTimeout,
                                    std::chrono::milliseconds readTimeout)
    : host_(std::move(host)), port_(port),
      connectTimeout_(connectTimeout), socket_(readTimeout) {}

VirtualConnector::~VirtualConnector() {
    socket_.close();
}

void VirtualConnector::setState(ConnectionState state) {
    state_ = state;
}

void VirtualConnector::discover() {
    // Simulateur a adresse fixe : rien a decouvrir dynamiquement.
    // Present pour respecter le contrat IConnector (section 21).
}

void VirtualConnector::connect() {
    setState(ConnectionState::CONNECTING);
    try {
        socket_.open(host_, port_, connectTimeout_);
    } catch (const std::exception& e) {
        setState(ConnectionState::ERROR);
        metrics_.recordError();
        logger_.log(LogLevel::ERROR, "VirtualConnector", "CONNECTION_FAILED", sessionId_,
                    {{"reason", e.what(), false}});
        throw ConnectorError(ErrorCode::CONNECTOR_NOT_FOUND, e.what(),
            Severity::ERROR, "VirtualConnector", "", "", nowIso8601());
    }
    setState(ConnectionState::CONNECTED);
    metrics_.recordConnection();
    logger_.log(LogLevel::INFO, "VirtualConnector", "CONNECTION_ESTABLISHED", sessionId_);
}

void VirtualConnector::disconnect() {
    setState(ConnectionState::DISCONNECTING);
    socket_.close();
    setState(ConnectionState::DISCONNECTED);
    logger_.log(LogLevel::INFO, "VirtualConnector", "SESSION_ENDED", sessionId_);
}

void VirtualConnector::send(const Command& command) {
    if (state_ != ConnectionState::CONNECTED) {
        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "send() appele hors connexion", Severity::ERROR,
            "VirtualConnector", "", command.correlationId, nowIso8601());
    }
    // Validation de securite AVANT toute ecriture reseau (section 19) :
    // une commande bloquee ne doit jamais partir vers le vehicule.
    securityPolicy_.validate(command);

    // Controle de debit (section 15) : leve BUFFER_OVERFLOW si le
    // rythme configure (defaut 20 req/s) est depasse.
    rateLimiter_.checkAndRecord();

    try {
        socket_.writeLine(command.payload);
    } catch (const std::exception& e) {
        setState(ConnectionState::DEGRADED);
        metrics_.recordError();
        throw ConnectorError(ErrorCode::WRITE_FAILED, e.what(),
            Severity::ERROR, "VirtualConnector", "", command.correlationId, nowIso8601());
    }

    metrics_.recordCommandSent();
    logger_.log(LogLevel::INFO, "VirtualConnector", "COMMAND_SENT", sessionId_,
                {{"command", command.payload, false}, {"commandId", command.commandId, false}});
}

RawVehicleMessage VirtualConnector::receive() {
    auto start = std::chrono::steady_clock::now();
    std::string payload;
    try {
        payload = socket_.readUntilPrompt();
    } catch (const std::exception& e) {
        std::string what = e.what();
        if (what == "TIMEOUT") {
            metrics_.recordTimeout();
            logger_.log(LogLevel::WARNING, "VirtualConnector", "COMMAND_TIMEOUT", sessionId_);
            throw ConnectorError(ErrorCode::TIMEOUT,
                "Aucune reponse recue dans le delai imparti", Severity::WARNING,
                "VirtualConnector", "", "", nowIso8601());
        }

        // Lien reellement perdu (le pair a ferme la connexion) : on
        // declenche la reconnexion automatique (section 13), pilotee
        // par retryPolicy_ (section 12). L'appelant original recoit une
        // erreur claire lui indiquant de renvoyer sa commande -- on ne
        // rejoue pas silencieusement une commande qui a pu se perdre
        // pendant la coupure.
        logger_.log(LogLevel::WARNING, "VirtualConnector", "ConnectionLost", sessionId_);
        vcsdk::retry::performReconnect(
            [this]() { socket_.close(); socket_.open(host_, port_, connectTimeout_); },
            [this](ConnectionState s) { setState(s); },
            retryPolicy_, "VirtualConnector", nowIso8601());

        metrics_.recordReconnection();
        logger_.log(LogLevel::INFO, "VirtualConnector", "ReconnectSucceeded", sessionId_);

        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "Lien perdu ; reconnexion automatique reussie -- veuillez renvoyer votre commande",
            Severity::WARNING, "VirtualConnector", "", "", nowIso8601());
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    RawVehicleMessage msg;
    msg.messageId = "MSG-" + std::to_string(++messageCounter_);
    msg.sessionId = sessionId_;
    msg.timestamp = nowIso8601();
    msg.protocol = "OBD2";
    msg.source = "ECU";
    msg.rawPayload = payload;
    msg.status = "SUCCESS";
    msg.latencyMs = static_cast<int>(elapsed.count());

    metrics_.recordMessageReceived(msg.latencyMs);
    logger_.log(LogLevel::INFO, "VirtualConnector", "MessageReceived", sessionId_,
                {{"messageId", msg.messageId, false}});

    // Buffer technique (section 14) : conserve les derniers messages,
    // sans jamais devenir un stockage metier -- taille bornee (100).
    buffer_.push(msg);

    return msg;
}

bool VirtualConnector::healthCheck() {
    return state_ == ConnectionState::CONNECTED && socket_.isOpen();
}

ConnectionState VirtualConnector::getStatus() const {
    return state_;
}

}  // namespace vcsdk::connectors

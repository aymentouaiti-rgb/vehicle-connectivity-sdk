#include "vcsdk/connectors/bluetooth/BluetoothConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
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

BluetoothConnector::BluetoothConnector(std::string macAddress, int channel,
                                        std::chrono::milliseconds readTimeout)
    : macAddress_(std::move(macAddress)), channel_(channel), readTimeout_(readTimeout) {}

BluetoothConnector::~BluetoothConnector() {
    if (socketFd_ >= 0) ::close(socketFd_);
}

void BluetoothConnector::setState(ConnectionState state) {
    state_ = state;
}

void BluetoothConnector::discover() {
    // TODO (post-MVP) : implementer un vrai inquiry BlueZ (hci_inquiry)
    // pour lister les peripheriques a proximite, comme illustre en
    // section 5 de la specification. Non fait au MVP faute de materiel
    // pour valider le resultat.
}

void BluetoothConnector::connect() {
    setState(ConnectionState::CONNECTING);

    socketFd_ = ::socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (socketFd_ < 0) {
        setState(ConnectionState::ERROR);
        throw ConnectorError(ErrorCode::CONNECTOR_NOT_FOUND,
            "Impossible de creer le socket Bluetooth (adaptateur absent ?)",
            Severity::ERROR, "BluetoothConnector", "", "", nowIso8601());
    }

    struct timeval tv;
    tv.tv_sec = static_cast<long>(readTimeout_.count() / 1000);
    tv.tv_usec = static_cast<long>((readTimeout_.count() % 1000) * 1000);
    ::setsockopt(socketFd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_rc addr{};
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = static_cast<uint8_t>(channel_);
    ::str2ba(macAddress_.c_str(), &addr.rc_bdaddr);

    if (::connect(socketFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(socketFd_);
        socketFd_ = -1;
        setState(ConnectionState::ERROR);
        throw ConnectorError(ErrorCode::CONNECTION_FAILED,
            "Connexion RFCOMM impossible vers " + macAddress_, Severity::ERROR,
            "BluetoothConnector", "", "", nowIso8601());
    }

    setState(ConnectionState::CONNECTED);
}

void BluetoothConnector::disconnect() {
    setState(ConnectionState::DISCONNECTING);
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
    }
    setState(ConnectionState::DISCONNECTED);
}

void BluetoothConnector::send(const Command& command) {
    if (state_ != ConnectionState::CONNECTED) {
        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "send() appele hors connexion", Severity::ERROR,
            "BluetoothConnector", "", command.correlationId, nowIso8601());
    }
    // Validation de securite AVANT toute emission (section 19).
    securityPolicy_.validate(command);
    std::string wire = command.payload + "\r";
    ssize_t n = ::send(socketFd_, wire.data(), wire.size(), 0);
    if (n < 0 || static_cast<size_t>(n) != wire.size()) {
        setState(ConnectionState::DEGRADED);
        throw ConnectorError(ErrorCode::WRITE_FAILED, "Echec d'ecriture RFCOMM",
            Severity::ERROR, "BluetoothConnector", "", command.correlationId, nowIso8601());
    }
}

RawVehicleMessage BluetoothConnector::receive() {
    auto start = std::chrono::steady_clock::now();

    while (recvBuffer_.find('>') == std::string::npos) {
        char chunk[512];
        ssize_t n = ::recv(socketFd_, chunk, sizeof(chunk), 0);
        if (n == 0) {
            setState(ConnectionState::DEGRADED);
            throw ConnectorError(ErrorCode::CONNECTION_LOST,
                "Connexion Bluetooth fermee par le pair", Severity::ERROR,
                "BluetoothConnector", "", "", nowIso8601());
        }
        if (n < 0) {
            throw ConnectorError(ErrorCode::TIMEOUT,
                "Aucune reponse recue dans le delai imparti", Severity::WARNING,
                "BluetoothConnector", "", "", nowIso8601());
        }
        recvBuffer_.append(chunk, static_cast<size_t>(n));
    }

    auto pos = recvBuffer_.find('>');
    std::string payload = recvBuffer_.substr(0, pos);
    recvBuffer_.erase(0, pos + 1);

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

bool BluetoothConnector::healthCheck() {
    return state_ == ConnectionState::CONNECTED;
}

ConnectionState BluetoothConnector::getStatus() const {
    return state_;
}

}  // namespace vcsdk::connectors

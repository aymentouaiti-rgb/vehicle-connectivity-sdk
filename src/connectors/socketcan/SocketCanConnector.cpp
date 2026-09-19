#include "vcsdk/connectors/socketcan/SocketCanConnector.hpp"
#include "vcsdk/core/ConnectorError.hpp"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>
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

// Parse "7DF#0201000000000000" -> struct can_frame.
can_frame parseCanFrame(const std::string& payload) {
    auto hashPos = payload.find('#');
    if (hashPos == std::string::npos) {
        throw std::runtime_error("payload CAN invalide (attendu 'ID#DATA'): " + payload);
    }
    can_frame frame{};
    frame.can_id = static_cast<canid_t>(std::stoul(payload.substr(0, hashPos), nullptr, 16));
    std::string dataHex = payload.substr(hashPos + 1);
    frame.can_dlc = static_cast<__u8>(dataHex.size() / 2);
    for (size_t i = 0; i < dataHex.size() / 2 && i < 8; ++i) {
        frame.data[i] = static_cast<__u8>(std::stoul(dataHex.substr(i * 2, 2), nullptr, 16));
    }
    return frame;
}

// Formate une struct can_frame recue -> "7DF#0201000000000000".
std::string formatCanFrame(const can_frame& frame) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << frame.can_id << "#";
    for (int i = 0; i < frame.can_dlc; ++i) {
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(frame.data[i]);
    }
    return oss.str();
}
}  // namespace

SocketCanConnector::SocketCanConnector(std::string interfaceName)
    : interfaceName_(std::move(interfaceName)) {}

SocketCanConnector::~SocketCanConnector() {
    if (socketFd_ >= 0) ::close(socketFd_);
}

void SocketCanConnector::setState(ConnectionState state) {
    state_ = state;
}

void SocketCanConnector::discover() {
    setState(ConnectionState::DISCOVERING);
    interfaceFound_ = (::if_nametoindex(interfaceName_.c_str()) != 0);
    setState(ConnectionState::DISCONNECTED);
}

void SocketCanConnector::connect() {
    setState(ConnectionState::CONNECTING);

    socketFd_ = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd_ < 0) {
        setState(ConnectionState::ERROR);
        throw ConnectorError(ErrorCode::CONNECTION_FAILED,
            "Impossible de creer le socket CAN (support noyau absent ?)",
            Severity::ERROR, "SocketCanConnector", "", "", nowIso8601());
    }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, interfaceName_.c_str(), IFNAMSIZ - 1);
    if (::ioctl(socketFd_, SIOCGIFINDEX, &ifr) < 0) {
        ::close(socketFd_);
        socketFd_ = -1;
        setState(ConnectionState::ERROR);
        throw ConnectorError(ErrorCode::CONNECTOR_NOT_FOUND,
            "Interface CAN introuvable: " + interfaceName_, Severity::ERROR,
            "SocketCanConnector", "", "", nowIso8601());
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (::bind(socketFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(socketFd_);
        socketFd_ = -1;
        setState(ConnectionState::ERROR);
        throw ConnectorError(ErrorCode::CONNECTION_FAILED,
            "bind() CAN echoue sur " + interfaceName_, Severity::ERROR,
            "SocketCanConnector", "", "", nowIso8601());
    }

    struct timeval tv{1, 0};  // 1s -- ameliorer via configuration (section 30)
    ::setsockopt(socketFd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    setState(ConnectionState::CONNECTED);
}

void SocketCanConnector::disconnect() {
    setState(ConnectionState::DISCONNECTING);
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
    }
    setState(ConnectionState::DISCONNECTED);
}

void SocketCanConnector::send(const Command& command) {
    if (state_ != ConnectionState::CONNECTED) {
        throw ConnectorError(ErrorCode::CONNECTION_LOST,
            "send() appele hors connexion", Severity::ERROR,
            "SocketCanConnector", "", command.correlationId, nowIso8601());
    }
    can_frame frame;
    try {
        frame = parseCanFrame(command.payload);
    } catch (const std::exception& e) {
        throw ConnectorError(ErrorCode::INVALID_RESPONSE, e.what(), Severity::ERROR,
            "SocketCanConnector", "", command.correlationId, nowIso8601());
    }
    ssize_t n = ::write(socketFd_, &frame, sizeof(frame));
    if (n != sizeof(frame)) {
        setState(ConnectionState::DEGRADED);
        throw ConnectorError(ErrorCode::WRITE_FAILED, "Echec d'ecriture trame CAN",
            Severity::ERROR, "SocketCanConnector", "", command.correlationId, nowIso8601());
    }
}

RawVehicleMessage SocketCanConnector::receive() {
    auto start = std::chrono::steady_clock::now();
    can_frame frame{};
    ssize_t n = ::read(socketFd_, &frame, sizeof(frame));
    if (n < 0) {
        throw ConnectorError(ErrorCode::TIMEOUT,
            "Aucune trame CAN recue dans le delai imparti", Severity::WARNING,
            "SocketCanConnector", "", "", nowIso8601());
    }
    if (n != sizeof(frame)) {
        throw ConnectorError(ErrorCode::INVALID_RESPONSE, "Trame CAN incomplete",
            Severity::ERROR, "SocketCanConnector", "", "", nowIso8601());
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    RawVehicleMessage msg;
    msg.messageId = "MSG-" + std::to_string(++messageCounter_);
    msg.timestamp = nowIso8601();
    msg.protocol = "CAN";
    msg.source = "ECU";
    msg.rawPayload = formatCanFrame(frame);
    msg.status = "SUCCESS";
    msg.latencyMs = static_cast<int>(elapsed.count());
    return msg;
}

bool SocketCanConnector::healthCheck() {
    return state_ == ConnectionState::CONNECTED && socketFd_ >= 0;
}

ConnectionState SocketCanConnector::getStatus() const {
    return state_;
}

}  // namespace vcsdk::connectors

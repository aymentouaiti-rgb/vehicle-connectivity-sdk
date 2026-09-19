#include "vcsdk/net/TcpSocketClient.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

namespace vcsdk::net {

TcpSocketClient::TcpSocketClient(std::chrono::milliseconds readTimeout)
    : readTimeout_(readTimeout) {}

TcpSocketClient::~TcpSocketClient() {
    close();
}

void TcpSocketClient::open(const std::string& host, int port,
                            std::chrono::milliseconds connectTimeout) {
    (void)connectTimeout;  // le timeout de connexion "propre" (non bloquant
                            // + select()) est une amelioration future ;
                            // pour le MVP on garde un connect() bloquant.

    socketFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ < 0) {
        throw std::runtime_error("socket() failed");
    }

    struct timeval tv;
    tv.tv_sec = static_cast<long>(readTimeout_.count() / 1000);
    tv.tv_usec = static_cast<long>((readTimeout_.count() % 1000) * 1000);
    ::setsockopt(socketFd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
        throw std::runtime_error("invalid address: " + host);
    }

    if (::connect(socketFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(socketFd_);
        socketFd_ = -1;
        throw std::runtime_error("connect failed to " + host + ":" + std::to_string(port));
    }
}

void TcpSocketClient::close() {
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
    }
    recvBuffer_.clear();
}

bool TcpSocketClient::isOpen() const {
    return socketFd_ >= 0;
}

void TcpSocketClient::writeLine(const std::string& payload) {
    if (!isOpen()) {
        throw std::runtime_error("write on closed socket");
    }
    std::string wire = payload + "\r";
    ssize_t n = ::send(socketFd_, wire.data(), wire.size(), 0);
    if (n < 0 || static_cast<size_t>(n) != wire.size()) {
        throw std::runtime_error("write failed");
    }
}

std::string TcpSocketClient::readUntilPrompt() {
    while (recvBuffer_.find('>') == std::string::npos) {
        char chunk[512];
        ssize_t n = ::recv(socketFd_, chunk, sizeof(chunk), 0);
        if (n == 0) {
            throw std::runtime_error("CLOSED");
        }
        if (n < 0) {
            throw std::runtime_error("TIMEOUT");
        }
        recvBuffer_.append(chunk, static_cast<size_t>(n));
    }
    auto pos = recvBuffer_.find('>');
    std::string payload = recvBuffer_.substr(0, pos);
    recvBuffer_.erase(0, pos + 1);
    return payload;
}

}  // namespace vcsdk::net

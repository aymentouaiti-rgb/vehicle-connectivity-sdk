#include "vcsdk/net/SerialPortClient.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstring>
#include <stdexcept>

namespace vcsdk::net {

namespace {
speed_t toSpeedConstant(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        default:     return B38400;  // valeur ELM327 la plus repandue
    }
}
}  // namespace

SerialPortClient::SerialPortClient(std::chrono::milliseconds readTimeout)
    : readTimeout_(readTimeout) {}

SerialPortClient::~SerialPortClient() {
    close();
}

void SerialPortClient::open(const std::string& devicePath, int baudRate) {
    fd_ = ::open(devicePath.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0) {
        throw std::runtime_error("cannot open serial device: " + devicePath);
    }

    struct termios tty{};
    if (::tcgetattr(fd_, &tty) != 0) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("tcgetattr failed on " + devicePath);
    }

    speed_t speed = toSpeedConstant(baudRate);
    ::cfsetospeed(&tty, speed);
    ::cfsetispeed(&tty, speed);

    // Mode "raw" : pas d'echo, pas de traitement de ligne canonique, pas
    // de traduction de caracteres -- on veut des octets bruts, exactement
    // ce que transporte le protocole ELM327 (lignes terminees par '\r').
    ::cfmakeraw(&tty);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;   // pas de bit de parite
    tty.c_cflag &= ~CSTOPB;   // 1 bit de stop
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;       // 8 bits de donnees (8N1)

    // VMIN=0, VTIME en dixiemes de seconde : lecture non bloquante avec
    // timeout, evite d'avoir a gerer un thread separe pour le MVP.
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = static_cast<cc_t>(readTimeout_.count() / 100);

    if (::tcsetattr(fd_, TCSANOW, &tty) != 0) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("tcsetattr failed on " + devicePath);
    }
}

void SerialPortClient::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    recvBuffer_.clear();
}

bool SerialPortClient::isOpen() const {
    return fd_ >= 0;
}

void SerialPortClient::writeLine(const std::string& payload) {
    if (!isOpen()) {
        throw std::runtime_error("write on closed serial port");
    }
    std::string wire = payload + "\r";
    ssize_t n = ::write(fd_, wire.data(), wire.size());
    if (n < 0 || static_cast<size_t>(n) != wire.size()) {
        throw std::runtime_error("serial write failed");
    }
}

std::string SerialPortClient::readUntilPrompt() {
    while (recvBuffer_.find('>') == std::string::npos) {
        char chunk[256];
        ssize_t n = ::read(fd_, chunk, sizeof(chunk));
        if (n < 0) {
            throw std::runtime_error("serial read error");
        }
        if (n == 0) {
            // VMIN=0/VTIME>0 : n==0 signifie "timeout ecoule, rien recu",
            // pas une deconnexion (contrairement a un socket TCP).
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

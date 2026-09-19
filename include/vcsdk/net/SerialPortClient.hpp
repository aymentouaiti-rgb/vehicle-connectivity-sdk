#pragma once

#include <string>
#include <chrono>

namespace vcsdk::net {

// Client port serie bas niveau, meme role que TcpSocketClient mais pour
// un transport /dev/ttyUSBx ou /dev/rfcommx (Bluetooth Classic expose
// aussi un port serie une fois jumele, cf. profil SPP).
class SerialPortClient {
public:
    explicit SerialPortClient(std::chrono::milliseconds readTimeout = std::chrono::milliseconds(1000));
    ~SerialPortClient();

    SerialPortClient(const SerialPortClient&) = delete;
    SerialPortClient& operator=(const SerialPortClient&) = delete;

    // Ouvre le port serie et le configure en mode brut (raw), 8N1,
    // au debit indique. Leve std::runtime_error en cas d'echec.
    void open(const std::string& devicePath, int baudRate);

    void close();
    bool isOpen() const;

    void writeLine(const std::string& payload);
    std::string readUntilPrompt();

private:
    int fd_ = -1;
    std::chrono::milliseconds readTimeout_;
    std::string recvBuffer_;
};

}  // namespace vcsdk::net

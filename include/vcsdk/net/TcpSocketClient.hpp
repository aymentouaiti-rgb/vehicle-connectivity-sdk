#pragma once

#include <string>
#include <chrono>

namespace vcsdk::net {

// Client TCP bas niveau, reutilisable par plusieurs connecteurs
// (VirtualConnector ET TcpConnector partagent la meme mecanique de
// transport -- seule leur ROLE dans l'architecture differe : l'un parle
// a un simulateur, l'autre a un vrai adaptateur reseau).
//
// Cette classe n'implemente PAS IConnector : c'est un outil de plus bas
// niveau, utilise par composition (pattern Adapter, section 22 de la spec)
// plutot que par heritage.
class TcpSocketClient {
public:
    explicit TcpSocketClient(std::chrono::milliseconds readTimeout = std::chrono::milliseconds(1000));
    ~TcpSocketClient();

    TcpSocketClient(const TcpSocketClient&) = delete;
    TcpSocketClient& operator=(const TcpSocketClient&) = delete;

    // Ouvre la connexion TCP. Leve std::runtime_error en cas d'echec
    // (le connecteur appelant se charge de le traduire en ConnectorError,
    // pour ne pas coupler cette classe bas niveau au catalogue d'erreurs
    // metier du SDK).
    void open(const std::string& host, int port,
              std::chrono::milliseconds connectTimeout);

    void close();
    bool isOpen() const;

    // Envoie une ligne brute terminee par '\r' (convention ELM327,
    // partagee par le simulateur ET la plupart des adaptateurs WiFi reels).
    void writeLine(const std::string& payload);

    // Lit jusqu'a rencontrer le caractere '>' (fin de reponse ELM327),
    // en accumulant les fragments reseau. Retourne le payload sans le '>'.
    // Leve std::runtime_error("TIMEOUT") ou std::runtime_error("CLOSED")
    // selon la cause.
    std::string readUntilPrompt();

private:
    int socketFd_ = -1;
    std::chrono::milliseconds readTimeout_;
    std::string recvBuffer_;
};

}  // namespace vcsdk::net

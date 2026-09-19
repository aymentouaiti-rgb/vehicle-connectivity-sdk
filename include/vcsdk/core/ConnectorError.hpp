#pragma once

#include <string>
#include <stdexcept>

namespace vcsdk::core {

// Niveau de gravite d'une erreur, expose dans chaque ConnectorError (section 16).
enum class Severity { INFO, WARNING, ERROR, CRITICAL };

// Catalogue d'erreurs impose par la specification (section 16).
// Utiliser ces codes exacts partout dans le SDK -- ne pas en inventer d'autres
// sans les ajouter ici, pour que le catalogue reste la source unique de verite.
namespace ErrorCode {
    inline constexpr const char* CONNECTOR_NOT_FOUND    = "CONNECTOR_NOT_FOUND";
    inline constexpr const char* CONNECTION_FAILED      = "CONNECTION_FAILED";
    inline constexpr const char* CONNECTION_LOST        = "CONNECTION_LOST";
    inline constexpr const char* TIMEOUT                = "TIMEOUT";
    inline constexpr const char* WRITE_FAILED            = "WRITE_FAILED";
    inline constexpr const char* READ_FAILED             = "READ_FAILED";
    inline constexpr const char* INVALID_RESPONSE        = "INVALID_RESPONSE";
    inline constexpr const char* BUFFER_OVERFLOW         = "BUFFER_OVERFLOW";
    inline constexpr const char* UNSUPPORTED_CONNECTOR   = "UNSUPPORTED_CONNECTOR";
    inline constexpr const char* PERMISSION_DENIED       = "PERMISSION_DENIED";
    inline constexpr const char* DEVICE_BUSY             = "DEVICE_BUSY";
}

// Exception normalisee du SDK. Chaque erreur DOIT exposer les 7 champs
// listes en section 16 -- c'est un critere d'acceptation (section 33).
class ConnectorError : public std::runtime_error {
public:
    ConnectorError(std::string code,
                    std::string message,
                    Severity severity,
                    std::string connectorId,
                    std::string sessionId,
                    std::string correlationId,
                    std::string timestamp)
        : std::runtime_error(message),
          code_(std::move(code)),
          message_(message),
          severity_(severity),
          connectorId_(std::move(connectorId)),
          sessionId_(std::move(sessionId)),
          correlationId_(std::move(correlationId)),
          timestamp_(std::move(timestamp)) {}

    const std::string& code() const noexcept { return code_; }
    const std::string& message() const noexcept { return message_; }
    Severity severity() const noexcept { return severity_; }
    const std::string& connectorId() const noexcept { return connectorId_; }
    const std::string& sessionId() const noexcept { return sessionId_; }
    const std::string& correlationId() const noexcept { return correlationId_; }
    const std::string& timestamp() const noexcept { return timestamp_; }

private:
    std::string code_;
    std::string message_;
    Severity severity_;
    std::string connectorId_;
    std::string sessionId_;
    std::string correlationId_;
    std::string timestamp_;
};

}  // namespace vcsdk::core

#pragma once

#include <string>
#include <optional>

namespace vcsdk::core {

// Objet standardise represantant une reponse brute du vehicule.
// IMPORTANT : le SDK ne doit JAMAIS interpreter rawPayload (section 2 et 9).
// C'est le Protocol Engine (DEV 2) qui decode "410C1AF8" en "1726 RPM".
struct RawVehicleMessage {
    std::string messageId;
    std::string sessionId;
    std::string timestamp;      // ISO 8601, ex: "2026-08-09T18:03:21Z"
    std::string protocol;       // ex: "OBD2"
    std::string source;         // ex: "ECU"
    std::string destination;
    std::string channel;
    std::string rawPayload;     // ex: "410C1AF8" -- jamais interprete ici
    std::string status;         // "SUCCESS" | "TIMEOUT" | "ERROR" ...
    int latencyMs = 0;
    std::optional<std::string> correlationId;
};

}  // namespace vcsdk::core

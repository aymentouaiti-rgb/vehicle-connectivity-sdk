#pragma once

#include <string>
#include <chrono>

namespace vcsdk::core {

// Les 4 niveaux de priorite imposes par la specification (section 10).
// Valeur numerique basse = priorite haute (convention std::priority_queue-friendly
// via un comparateur inverse, voir CommandQueue.hpp).
enum class CommandPriority {
    Emergency = 1,   // Priority 1
    Diagnostic = 2,  // Priority 2
    Telemetry = 3,   // Priority 3
    Background = 4   // Priority 4
};

// Une commande a transporter vers le vehicule. Le SDK ne doit jamais
// interpreter le contenu de "payload" (section 8) -- ex: "010C" reste
// une chaine opaque pour DEV 1.
struct Command {
    std::string commandId;
    std::string payload;
    CommandPriority priority = CommandPriority::Diagnostic;
    std::chrono::milliseconds timeout{2000};  // valeur par defaut section 11
    int maxAttempts = 1;
    std::string correlationId;
};

}  // namespace vcsdk::core

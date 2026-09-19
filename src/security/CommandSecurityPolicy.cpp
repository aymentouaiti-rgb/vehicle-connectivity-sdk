#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/core/ConnectorError.hpp"

#include <algorithm>
#include <cctype>

namespace vcsdk::security {

using namespace vcsdk::core;

namespace {
std::string toUpperTrimmed(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }
    return out;
}
}  // namespace

bool CommandSecurityPolicy::isKnownModifyingCommand(const std::string& payload) {
    std::string p = toUpperTrimmed(payload);

    // OBD-II Mode 04 : effacement des codes DTC -- exemple explicitement
    // cite en section 19 ("effacement DTC").
    if (p == "04") return true;

    // Services UDS (ISO 14229) potentiellement destructeurs, cites ou
    // impliques par les exemples de la section 19 :
    //   11 = ECUReset               ("reset sensible")
    //   14 = ClearDiagnosticInformation ("effacement DTC", variante UDS)
    //   27 = SecurityAccess         (prealable a "programmation ECU")
    //   2E = WriteDataByIdentifier  ("modification de configuration")
    //   31 = RoutineControl         ("actionneur")
    //   3D = WriteMemoryByAddress   ("programmation ECU")
    static const char* kDangerousPrefixes[] = {"11", "14", "27", "2E", "31", "3D"};
    for (const char* prefix : kDangerousPrefixes) {
        if (p.rfind(prefix, 0) == 0) return true;  // p commence par prefix
    }

    return false;
}

void CommandSecurityPolicy::validate(const Command& command) const {
    if (mode_ == SdkMode::FULL) return;  // toutes les commandes autorisees

    if (isKnownModifyingCommand(command.payload)) {
        throw ConnectorError(
            ErrorCode::PERMISSION_DENIED,
            "Commande bloquee : le SDK est en mode READ_ONLY (section 19)",
            Severity::WARNING, "CommandSecurityPolicy", "", command.correlationId, "");
    }
}

}  // namespace vcsdk::security

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <ctime>
#include <chrono>

namespace vcsdk::logging {

// Les 5 niveaux imposes par la section 17.
enum class LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

inline const char* toString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

// Une paire cle/valeur de contexte additionnel (ex: {"command", "010C"}).
struct LogField {
    std::string key;
    std::string value;
    bool sensitive = false;  // section 17 : "les donnees sensibles
                              // doivent pouvoir etre masquees"
};

// Logger structure minimal (section 17). Produit une ligne JSON par
// evenement, exactement dans l'esprit de l'exemple de la specification :
//   {"timestamp":"...","level":"INFO","component":"BluetoothConnector",
//    "sessionId":"...","event":"COMMAND_SENT","command":"010C"}
//
// Volontairement simple (pas de rotation de fichiers, pas de sinks
// multiples) -- suffisant pour le MVP ; une bibliotheque comme spdlog
// pourrait remplacer cette implementation sans changer les appelants,
// grace a l'interface etroite (une seule methode log()).
class StructuredLogger {
public:
    explicit StructuredLogger(std::ostream& out = std::cout) : out_(out) {}

    void log(LogLevel level, const std::string& component,
              const std::string& event, const std::string& sessionId = "",
              const std::vector<LogField>& fields = {}) {
        if (static_cast<int>(level) < static_cast<int>(minLevel_)) return;

        std::lock_guard<std::mutex> lock(mutex_);
        out_ << "{"
             << "\"timestamp\":\"" << nowIso8601() << "\","
             << "\"level\":\"" << toString(level) << "\","
             << "\"component\":\"" << component << "\",";
        if (!sessionId.empty()) {
            out_ << "\"sessionId\":\"" << sessionId << "\",";
        }
        out_ << "\"event\":\"" << event << "\"";
        for (const auto& f : fields) {
            out_ << ",\"" << f.key << "\":\""
                 << (f.sensitive ? "***MASKED***" : f.value) << "\"";
        }
        out_ << "}\n";
    }

    void setMinLevel(LogLevel level) { minLevel_ = level; }

private:
    static std::string nowIso8601() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
        return std::string(buf);
    }

    std::ostream& out_;
    LogLevel minLevel_ = LogLevel::DEBUG;
    std::mutex mutex_;
};

}  // namespace vcsdk::logging

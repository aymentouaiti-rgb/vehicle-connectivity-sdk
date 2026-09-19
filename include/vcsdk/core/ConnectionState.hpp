#pragma once

namespace vcsdk::core {

// Les 8 etats de connexion imposes par la specification (section 6).
// enum class : pas de conversion implicite en int, pas de collision de noms
// (ConnectionState::ERROR ne rentre pas en conflit avec une macro ERROR par ex.)
enum class ConnectionState {
    DISCONNECTED,
    DISCOVERING,
    CONNECTING,
    CONNECTED,
    DEGRADED,
    RECONNECTING,
    ERROR,
    DISCONNECTING
};

// Conversion en texte, utile pour les logs structures (section 17) et le JSON.
inline const char* toString(ConnectionState state) {
    switch (state) {
        case ConnectionState::DISCONNECTED:  return "DISCONNECTED";
        case ConnectionState::DISCOVERING:   return "DISCOVERING";
        case ConnectionState::CONNECTING:    return "CONNECTING";
        case ConnectionState::CONNECTED:     return "CONNECTED";
        case ConnectionState::DEGRADED:      return "DEGRADED";
        case ConnectionState::RECONNECTING:  return "RECONNECTING";
        case ConnectionState::ERROR:         return "ERROR";
        case ConnectionState::DISCONNECTING: return "DISCONNECTING";
    }
    return "UNKNOWN";
}

}  // namespace vcsdk::core

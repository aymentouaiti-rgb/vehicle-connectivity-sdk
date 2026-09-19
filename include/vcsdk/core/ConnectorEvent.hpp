#pragma once

#include <string>
#include <chrono>

namespace vcsdk::core {

// Catalogue minimal d'evenements impose par la specification (section 23).
enum class EventType {
    ConnectorDiscovered,
    ConnectionStarted,
    ConnectionEstablished,
    ConnectionFailed,
    ConnectionLost,
    ReconnectStarted,
    ReconnectSucceeded,
    ReconnectFailed,
    CommandQueued,
    CommandSent,
    MessageReceived,
    CommandTimeout,
    ConnectorErrorEvent,
    SessionStarted,
    SessionEnded
};

inline const char* toString(EventType type) {
    switch (type) {
        case EventType::ConnectorDiscovered:  return "ConnectorDiscovered";
        case EventType::ConnectionStarted:    return "ConnectionStarted";
        case EventType::ConnectionEstablished:return "ConnectionEstablished";
        case EventType::ConnectionFailed:     return "ConnectionFailed";
        case EventType::ConnectionLost:       return "ConnectionLost";
        case EventType::ReconnectStarted:     return "ReconnectStarted";
        case EventType::ReconnectSucceeded:   return "ReconnectSucceeded";
        case EventType::ReconnectFailed:      return "ReconnectFailed";
        case EventType::CommandQueued:        return "CommandQueued";
        case EventType::CommandSent:          return "CommandSent";
        case EventType::MessageReceived:      return "MessageReceived";
        case EventType::CommandTimeout:       return "CommandTimeout";
        case EventType::ConnectorErrorEvent:  return "ConnectorError";
        case EventType::SessionStarted:       return "SessionStarted";
        case EventType::SessionEnded:         return "SessionEnded";
    }
    return "UNKNOWN";
}

// Un evenement publie sur l'Event Bus (pattern Observer, section 22).
struct ConnectorEvent {
    EventType type;
    std::string sessionId;
    std::string connectorId;
    std::string detail;
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
};

}  // namespace vcsdk::core

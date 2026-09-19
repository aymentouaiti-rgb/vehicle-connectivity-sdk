#pragma once

#include <atomic>
#include <sstream>
#include <iomanip>

namespace vcsdk::monitoring {

// Les metriques techniques imposees par la section 18. std::atomic
// partout : plusieurs connecteurs/threads peuvent incrementer les
// memes compteurs sans synchronisation manuelle.
class Metrics {
public:
    void recordConnection() { connections_++; }
    void recordReconnection() { reconnections_++; }
    void recordCommandSent() { commandsSent_++; }
    void recordMessageReceived(int latencyMs) {
        messagesReceived_++;
        totalLatencyMs_ += latencyMs;
        int currentMax = maxLatencyMs_.load();
        while (latencyMs > currentMax &&
               !maxLatencyMs_.compare_exchange_weak(currentMax, latencyMs)) {
        }
    }
    void recordTimeout() { timeouts_++; }
    void recordError() { errors_++; }

    long long connections() const { return connections_.load(); }
    long long reconnections() const { return reconnections_.load(); }
    long long commandsSent() const { return commandsSent_.load(); }
    long long messagesReceived() const { return messagesReceived_.load(); }
    long long timeouts() const { return timeouts_.load(); }
    long long errors() const { return errors_.load(); }
    int maxLatencyMs() const { return maxLatencyMs_.load(); }

    double averageLatencyMs() const {
        long long count = messagesReceived_.load();
        if (count == 0) return 0.0;
        return static_cast<double>(totalLatencyMs_.load()) / static_cast<double>(count);
    }

    // Taux d'echec = (timeouts + erreurs) / commandes envoyees.
    double failureRate() const {
        long long sent = commandsSent_.load();
        if (sent == 0) return 0.0;
        return static_cast<double>(timeouts_.load() + errors_.load()) / static_cast<double>(sent);
    }

    // Rapport texte, dans l'esprit de l'exemple de la section 18.
    std::string report() const {
        std::ostringstream oss;
        oss << "Commands sent      : " << commandsSent() << "\n"
            << "Responses received : " << messagesReceived() << "\n"
            << "Timeouts           : " << timeouts() << "\n"
            << "Errors             : " << errors() << "\n"
            << "Average latency    : " << std::fixed << std::setprecision(1)
            << averageLatencyMs() << " ms\n"
            << "Max latency        : " << maxLatencyMs() << " ms\n"
            << "Failure rate       : " << std::fixed << std::setprecision(2)
            << (failureRate() * 100.0) << " %\n"
            << "Reconnections      : " << reconnections() << "\n";
        return oss.str();
    }

private:
    std::atomic<long long> connections_{0};
    std::atomic<long long> reconnections_{0};
    std::atomic<long long> commandsSent_{0};
    std::atomic<long long> messagesReceived_{0};
    std::atomic<long long> timeouts_{0};
    std::atomic<long long> errors_{0};
    std::atomic<long long> totalLatencyMs_{0};
    std::atomic<int> maxLatencyMs_{0};
};

}  // namespace vcsdk::monitoring

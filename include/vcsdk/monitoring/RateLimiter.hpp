#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include "vcsdk/core/ConnectorError.hpp"

namespace vcsdk::monitoring {

// Controle le debit de commandes (section 15). Exemple donne par la
// specification : "Maximum command rate: 20 requests/s". Implemente en
// fenetre glissante (sliding window) plutot qu'un simple compteur par
// seconde, pour eviter l'effet de bord classique "40 requetes d'un
// coup a cheval sur 2 secondes" que produirait un compteur naif.
class RateLimiter {
public:
    explicit RateLimiter(int maxRequestsPerSecond = 20)
        : maxRequestsPerSecond_(maxRequestsPerSecond) {}

    // Leve ConnectorError si la limite est depassee. A appeler juste
    // avant d'emettre une commande.
    void checkAndRecord() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto oneSecondAgo = now - std::chrono::seconds(1);

        while (!timestamps_.empty() && timestamps_.front() < oneSecondAgo) {
            timestamps_.pop_front();
        }

        if (static_cast<int>(timestamps_.size()) >= maxRequestsPerSecond_) {
            throw vcsdk::core::ConnectorError(
                vcsdk::core::ErrorCode::BUFFER_OVERFLOW,
                "Command rate limit exceeded (" + std::to_string(maxRequestsPerSecond_) + "/s)",
                vcsdk::core::Severity::WARNING, "RateLimiter", "", "", "");
        }

        timestamps_.push_back(now);
    }

    int getMaxRequestsPerSecond() const { return maxRequestsPerSecond_; }
    void setMaxRequestsPerSecond(int value) { maxRequestsPerSecond_ = value; }

    size_t currentRate() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return timestamps_.size();
    }

private:
    mutable std::mutex mutex_;
    std::deque<std::chrono::steady_clock::time_point> timestamps_;
    int maxRequestsPerSecond_;
};

}  // namespace vcsdk::monitoring

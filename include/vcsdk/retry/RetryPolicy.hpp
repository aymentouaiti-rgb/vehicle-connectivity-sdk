#pragma once

#include <random>
#include <algorithm>
#include "vcsdk/core/IConnectionPolicy.hpp"

namespace vcsdk::retry {

// Les 2 strategies recommandees par la section 12 de la specification.
enum class RetryStrategy { Fixed, Exponential };

// Implementation concrete de IConnectionPolicy. Exemple par defaut
// (3 tentatives, 500ms/1s/2s) correspond exactement a l'exemple donne
// en section 12 :
//   Attempt 1 -> echec -> 500ms -> Attempt 2 -> echec -> 1s -> Attempt 3
class RetryPolicy : public vcsdk::core::IConnectionPolicy {
public:
    explicit RetryPolicy(int maxAttempts = 3,
                          std::chrono::milliseconds baseDelay = std::chrono::milliseconds(500),
                          RetryStrategy strategy = RetryStrategy::Exponential,
                          double jitterRatio = 0.0)  // 0.2 = +/-20% de variation
        : maxAttempts_(maxAttempts), baseDelay_(baseDelay),
          strategy_(strategy), jitterRatio_(jitterRatio) {}

    bool shouldReconnect(int attemptNumber) const override {
        return attemptNumber < maxAttempts_;
    }

    std::chrono::milliseconds getNextRetryDelay(int attemptNumber) const override {
        long long ms = baseDelay_.count();
        if (strategy_ == RetryStrategy::Exponential && attemptNumber > 1) {
            // 500, 1000, 2000, 4000... (plafonne pour eviter un overflow
            // improbable sur un tres grand nombre de tentatives)
            int shift = std::min(attemptNumber - 1, 20);
            ms = baseDelay_.count() * (1LL << shift);
        }
        if (jitterRatio_ > 0.0) {
            std::uniform_real_distribution<double> dist(-jitterRatio_, jitterRatio_);
            double factor = 1.0 + dist(rng_);
            ms = static_cast<long long>(static_cast<double>(ms) * factor);
        }
        return std::chrono::milliseconds(ms);
    }

    int getMaxAttempts() const override { return maxAttempts_; }

private:
    int maxAttempts_;
    std::chrono::milliseconds baseDelay_;
    RetryStrategy strategy_;
    double jitterRatio_;
    mutable std::mt19937 rng_{std::random_device{}()};
};

}  // namespace vcsdk::retry

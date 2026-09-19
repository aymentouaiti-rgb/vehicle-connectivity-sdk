#include <gtest/gtest.h>
#include "vcsdk/retry/RetryPolicy.hpp"

using namespace vcsdk::retry;

TEST(RetryPolicy, ShouldReconnectRespecteMaxAttempts) {
    RetryPolicy policy(3, std::chrono::milliseconds(500));
    EXPECT_TRUE(policy.shouldReconnect(0));
    EXPECT_TRUE(policy.shouldReconnect(1));
    EXPECT_TRUE(policy.shouldReconnect(2));
    EXPECT_FALSE(policy.shouldReconnect(3));  // 3 tentatives max, deja faites
}

// Reproduit exactement l'exemple de la section 12 :
// Attempt 1 -> 500ms -> Attempt 2 -> 1s -> Attempt 3
TEST(RetryPolicy, ExponentialBackoffSuitLExempleDeLaSpec) {
    RetryPolicy policy(3, std::chrono::milliseconds(500), RetryStrategy::Exponential);
    EXPECT_EQ(policy.getNextRetryDelay(1).count(), 500);
    EXPECT_EQ(policy.getNextRetryDelay(2).count(), 1000);
    EXPECT_EQ(policy.getNextRetryDelay(3).count(), 2000);
}

TEST(RetryPolicy, FixedStrategyGardeLeMemeDelai) {
    RetryPolicy policy(5, std::chrono::milliseconds(300), RetryStrategy::Fixed);
    EXPECT_EQ(policy.getNextRetryDelay(1).count(), 300);
    EXPECT_EQ(policy.getNextRetryDelay(4).count(), 300);
}

TEST(RetryPolicy, JitterResteDansLaPlageAttendue) {
    RetryPolicy policy(3, std::chrono::milliseconds(1000), RetryStrategy::Fixed, /*jitter*/ 0.2);
    for (int i = 0; i < 50; ++i) {
        auto delay = policy.getNextRetryDelay(1).count();
        EXPECT_GE(delay, 800);   // 1000 - 20%
        EXPECT_LE(delay, 1200);  // 1000 + 20%
    }
}

TEST(RetryPolicy, GetMaxAttemptsRetourneLaValeurConfiguree) {
    RetryPolicy policy(7);
    EXPECT_EQ(policy.getMaxAttempts(), 7);
}

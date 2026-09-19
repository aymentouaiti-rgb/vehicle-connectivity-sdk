#include <gtest/gtest.h>
#include "vcsdk/monitoring/RateLimiter.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::monitoring;
using namespace vcsdk::core;

TEST(RateLimiter, AutoriseJusquauMaximumConfigure) {
    RateLimiter limiter(5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(limiter.checkAndRecord());
    }
}

TEST(RateLimiter, BloqueAuDelaDuMaximum) {
    RateLimiter limiter(5);
    for (int i = 0; i < 5; ++i) {
        limiter.checkAndRecord();
    }
    EXPECT_THROW(limiter.checkAndRecord(), ConnectorError);
}

TEST(RateLimiter, ValeurParDefautCorrespondALaSpec) {
    // Section 15 : "Maximum command rate: 20 requests/s"
    RateLimiter limiter;
    EXPECT_EQ(limiter.getMaxRequestsPerSecond(), 20);
}

TEST(RateLimiter, CurrentRateRefleteLesAppelsRecents) {
    RateLimiter limiter(10);
    limiter.checkAndRecord();
    limiter.checkAndRecord();
    EXPECT_EQ(limiter.currentRate(), 2u);
}

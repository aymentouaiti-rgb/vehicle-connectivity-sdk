// Tests unitaires de performReconnect(), isoles de tout vrai reseau --
// on injecte des callbacks factices pour tester les 2 issues possibles
// de la machine d'etat (section 6) :
//   RECONNECTING -> CONNECTED  (succes)
//   RECONNECTING -> ERROR      (echec apres epuisement des tentatives)

#include <gtest/gtest.h>
#include <vector>
#include "vcsdk/retry/ReconnectHelper.hpp"
#include "vcsdk/retry/RetryPolicy.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::core;
using namespace vcsdk::retry;

TEST(ReconnectHelper, ReussitDesLaPremiereTentative) {
    RetryPolicy fastPolicy(3, std::chrono::milliseconds(1));  // delais courts pour un test rapide
    std::vector<ConnectionState> statesSeen;

    int connectCallCount = 0;
    auto tryConnect = [&]() { connectCallCount++; /* succes immediat */ };
    auto setState = [&](ConnectionState s) { statesSeen.push_back(s); };

    EXPECT_NO_THROW(performReconnect(tryConnect, setState, fastPolicy, "Test", "now"));
    EXPECT_EQ(connectCallCount, 1);
    ASSERT_EQ(statesSeen.size(), 3u);
    EXPECT_EQ(statesSeen[0], ConnectionState::DEGRADED);
    EXPECT_EQ(statesSeen[1], ConnectionState::RECONNECTING);
    EXPECT_EQ(statesSeen[2], ConnectionState::CONNECTED);
}

TEST(ReconnectHelper, ReussitApresPlusieursEchecs) {
    RetryPolicy fastPolicy(5, std::chrono::milliseconds(1));
    int connectCallCount = 0;
    auto tryConnect = [&]() {
        connectCallCount++;
        if (connectCallCount < 3) throw std::runtime_error("simulated failure");
    };
    auto setState = [](ConnectionState) {};

    EXPECT_NO_THROW(performReconnect(tryConnect, setState, fastPolicy, "Test", "now"));
    EXPECT_EQ(connectCallCount, 3);  // echoue 2 fois, reussit a la 3e
}

// Scenario section 25 : "Reconnexion impossible -- 3 retries -> ERROR"
TEST(ReconnectHelper, EchoueApresEpuisementDesTentatives) {
    RetryPolicy fastPolicy(3, std::chrono::milliseconds(1));
    std::vector<ConnectionState> statesSeen;
    int connectCallCount = 0;

    auto tryConnect = [&]() {
        connectCallCount++;
        throw std::runtime_error("toujours en echec");
    };
    auto setState = [&](ConnectionState s) { statesSeen.push_back(s); };

    EXPECT_THROW(performReconnect(tryConnect, setState, fastPolicy, "Test", "now"), ConnectorError);
    EXPECT_EQ(connectCallCount, 3);  // exactement maxAttempts tentatives, pas plus
    EXPECT_EQ(statesSeen.back(), ConnectionState::ERROR);
}

TEST(ReconnectHelper, ErreurFinaleContientLeBonCode) {
    RetryPolicy fastPolicy(1, std::chrono::milliseconds(1));
    auto tryConnect = [&]() { throw std::runtime_error("echec"); };
    auto setState = [](ConnectionState) {};

    try {
        performReconnect(tryConnect, setState, fastPolicy, "MyConnector", "now");
        FAIL() << "une exception etait attendue";
    } catch (const ConnectorError& e) {
        EXPECT_STREQ(e.code().c_str(), ErrorCode::CONNECTION_LOST);
        EXPECT_EQ(e.connectorId(), "MyConnector");
    }
}

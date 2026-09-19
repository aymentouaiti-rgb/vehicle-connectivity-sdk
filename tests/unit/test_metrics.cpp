#include <gtest/gtest.h>
#include "vcsdk/monitoring/Metrics.hpp"

using namespace vcsdk::monitoring;

TEST(Metrics, CompteursDemarrentAZero) {
    Metrics m;
    EXPECT_EQ(m.commandsSent(), 0);
    EXPECT_EQ(m.messagesReceived(), 0);
    EXPECT_EQ(m.timeouts(), 0);
}

TEST(Metrics, IncrementeCorrectement) {
    Metrics m;
    m.recordCommandSent();
    m.recordCommandSent();
    m.recordMessageReceived(50);
    m.recordTimeout();
    EXPECT_EQ(m.commandsSent(), 2);
    EXPECT_EQ(m.messagesReceived(), 1);
    EXPECT_EQ(m.timeouts(), 1);
}

TEST(Metrics, LatenceMoyenneCorrecte) {
    Metrics m;
    m.recordMessageReceived(10);
    m.recordMessageReceived(20);
    m.recordMessageReceived(30);
    EXPECT_DOUBLE_EQ(m.averageLatencyMs(), 20.0);
}

TEST(Metrics, LatenceMaxSuitLePic) {
    Metrics m;
    m.recordMessageReceived(10);
    m.recordMessageReceived(99);
    m.recordMessageReceived(5);
    EXPECT_EQ(m.maxLatencyMs(), 99);
}

TEST(Metrics, TauxDEchecCalculeCorrectement) {
    Metrics m;
    m.recordCommandSent();
    m.recordCommandSent();
    m.recordCommandSent();
    m.recordCommandSent();
    m.recordTimeout();  // 1 echec sur 4 commandes = 25%
    EXPECT_DOUBLE_EQ(m.failureRate(), 0.25);
}

TEST(Metrics, ReportNeCrashePas) {
    Metrics m;
    m.recordCommandSent();
    m.recordMessageReceived(15);
    std::string report = m.report();
    EXPECT_FALSE(report.empty());
}

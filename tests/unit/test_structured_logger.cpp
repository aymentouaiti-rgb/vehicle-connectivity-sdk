#include <gtest/gtest.h>
#include <sstream>
#include "vcsdk/logging/StructuredLogger.hpp"

using namespace vcsdk::logging;

TEST(StructuredLogger, ProduitDuJsonValideBasique) {
    std::ostringstream oss;
    StructuredLogger logger(oss);
    logger.log(LogLevel::INFO, "TestComponent", "TEST_EVENT", "SESSION-1");

    std::string output = oss.str();
    EXPECT_NE(output.find("\"level\":\"INFO\""), std::string::npos);
    EXPECT_NE(output.find("\"component\":\"TestComponent\""), std::string::npos);
    EXPECT_NE(output.find("\"event\":\"TEST_EVENT\""), std::string::npos);
    EXPECT_NE(output.find("\"sessionId\":\"SESSION-1\""), std::string::npos);
}

TEST(StructuredLogger, IncludeLesChampsAdditionnels) {
    std::ostringstream oss;
    StructuredLogger logger(oss);
    logger.log(LogLevel::INFO, "Comp", "COMMAND_SENT", "S1",
                {{"command", "010C", false}});

    EXPECT_NE(oss.str().find("\"command\":\"010C\""), std::string::npos);
}

TEST(StructuredLogger, MasqueLesDonneesSensibles) {
    std::ostringstream oss;
    StructuredLogger logger(oss);
    logger.log(LogLevel::INFO, "Comp", "EVENT", "S1",
                {{"secret", "valeur-privee", true}});

    std::string output = oss.str();
    EXPECT_EQ(output.find("valeur-privee"), std::string::npos);
    EXPECT_NE(output.find("MASKED"), std::string::npos);
}

TEST(StructuredLogger, RespecteLeNiveauMinimum) {
    std::ostringstream oss;
    StructuredLogger logger(oss);
    logger.setMinLevel(LogLevel::ERROR);

    logger.log(LogLevel::DEBUG, "Comp", "IGNORED", "S1");
    logger.log(LogLevel::INFO, "Comp", "IGNORED_AUSSI", "S1");
    logger.log(LogLevel::ERROR, "Comp", "VISIBLE", "S1");

    std::string output = oss.str();
    EXPECT_EQ(output.find("IGNORED"), std::string::npos);
    EXPECT_NE(output.find("VISIBLE"), std::string::npos);
}

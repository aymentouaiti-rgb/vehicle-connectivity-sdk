#include <gtest/gtest.h>
#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::core;
using namespace vcsdk::security;

// Critere d'acceptation section 33 : "le mode lecture seule est actif
// par defaut" -- verifie explicitement ici, pas suppose.
TEST(CommandSecurityPolicy, ReadOnlyEstLeModeParDefaut) {
    CommandSecurityPolicy policy;
    EXPECT_EQ(policy.mode(), SdkMode::READ_ONLY);
}

TEST(CommandSecurityPolicy, BloqueEffacementDTC_Mode04) {
    Command cmd;
    cmd.payload = "04";
    CommandSecurityPolicy policy;  // READ_ONLY par defaut
    EXPECT_THROW(policy.validate(cmd), ConnectorError);
}

TEST(CommandSecurityPolicy, BloqueServicesUDSDangereux) {
    CommandSecurityPolicy policy;
    for (const char* payload : {"11", "1103", "14FF", "2E1234", "31017E01", "3D001000"}) {
        Command cmd;
        cmd.payload = payload;
        EXPECT_THROW(policy.validate(cmd), ConnectorError)
            << "aurait du etre bloque: " << payload;
    }
}

TEST(CommandSecurityPolicy, AutoriseLesCommandesDeLectureConnues) {
    CommandSecurityPolicy policy;
    for (const char* payload : {"010C", "010D", "0105", "03", "0902"}) {
        Command cmd;
        cmd.payload = payload;
        EXPECT_NO_THROW(policy.validate(cmd)) << "n'aurait pas du etre bloque: " << payload;
    }
}

TEST(CommandSecurityPolicy, ModeFullAutoriseTout) {
    CommandSecurityPolicy policy(SdkMode::FULL);
    Command cmd;
    cmd.payload = "04";  // normalement bloque en READ_ONLY
    EXPECT_NO_THROW(policy.validate(cmd));
}

TEST(CommandSecurityPolicy, SetModePermetDeChangerDynamiquement) {
    CommandSecurityPolicy policy;
    Command cmd;
    cmd.payload = "04";
    EXPECT_THROW(policy.validate(cmd), ConnectorError);

    policy.setMode(SdkMode::FULL);
    EXPECT_NO_THROW(policy.validate(cmd));

    policy.setMode(SdkMode::READ_ONLY);
    EXPECT_THROW(policy.validate(cmd), ConnectorError);
}

TEST(CommandSecurityPolicy, ErreurExposeLeCodePermissionDenied) {
    CommandSecurityPolicy policy;
    Command cmd;
    cmd.payload = "04";
    cmd.correlationId = "CORR-42";
    try {
        policy.validate(cmd);
        FAIL() << "une exception etait attendue";
    } catch (const ConnectorError& e) {
        EXPECT_STREQ(e.code().c_str(), ErrorCode::PERMISSION_DENIED);
        EXPECT_EQ(e.correlationId(), "CORR-42");
    }
}

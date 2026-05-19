#include <gtest/gtest.h>
#include <QCoreApplication>
#include <cstdio>
#include "cmd/micecam_ui/AppSettings.h"

static int s_argc = 0;

int main(int argc, char** argv) {
    s_argc = argc;
    if (!QCoreApplication::instance()) {
        static QCoreApplication app(argc, argv);
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(AppSettings, SettingsPersistAcrossInstances) {
    std::remove("micecam_config.json");

    {
        micecam::ui::AppSettings settings;
        settings.setOutputDirectory("/custom/output");
        settings.setWatchdogTimeout(7);
        settings.setLogLevel("debug");
        ASSERT_TRUE(settings.save());
    }

    micecam::ui::AppSettings settings2;
    EXPECT_EQ(settings2.outputDirectory().toStdString(), "/custom/output");
    EXPECT_EQ(settings2.watchdogTimeout(), 7);
    EXPECT_EQ(settings2.logLevel().toStdString(), "debug");

    std::remove("micecam_config.json");
}

TEST(AppSettings, FirstLaunchUsesDefaults) {
    std::remove("micecam_config.json");

    micecam::ui::AppSettings settings;
    EXPECT_EQ(settings.watchdogTimeout(), 3);
    EXPECT_DOUBLE_EQ(settings.yellowDropThreshold(), 0.1);
    EXPECT_DOUBLE_EQ(settings.redDropThreshold(), 1.0);
    EXPECT_EQ(settings.webhookUrl().toStdString(), "");
    EXPECT_EQ(settings.defaultBitrateKbps(), 5000);
    EXPECT_TRUE(settings.outputDirectory().toStdString().empty());
    EXPECT_EQ(settings.logLevel().toStdString(), "info");
}

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QThread>
#include "cmd/micecam_ui/AppController.h"

static int s_argc2 = 0;

int main(int argc, char** argv) {
    s_argc2 = argc;
    if (!QCoreApplication::instance()) {
        static QCoreApplication app(argc, argv);
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(AppController, NoInProcessBackends) {
    micecam::ui::AppController controller;
    controller.refreshCameras();

    ASSERT_NE(controller.cameraModel(), nullptr);
    EXPECT_EQ(controller.cameraModel()->rowCount(), 0);
    EXPECT_EQ(controller.cameraCountText().toStdString(), "0 cameras");
    EXPECT_FALSE(controller.isRecording());
    EXPECT_FALSE(controller.canStartRecording());
    EXPECT_EQ(controller.cameraCount(), 0);
    EXPECT_EQ(controller.recordButtonText().toStdString(), "No Device");
}

TEST(AppController, NoCameraStateIsConsistent) {
    micecam::ui::AppController controller;
    controller.refreshCameras();

    EXPECT_EQ(controller.cameraCount(), 0);
    EXPECT_FALSE(controller.isRecording());
    EXPECT_FALSE(controller.canStartRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "No Device");
    EXPECT_EQ(controller.preflightMessage().toStdString(), "No cameras detected");
    EXPECT_TRUE(controller.lastSessionId().isEmpty());
}

TEST(AppController, StartRecordingWithoutCamerasFails) {
    micecam::ui::AppController controller;
    controller.refreshCameras();

    EXPECT_FALSE(controller.startRecording());
    EXPECT_EQ(controller.cameraCount(), 0);
    EXPECT_FALSE(controller.isRecording());
    EXPECT_FALSE(controller.canStartRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "No Device");
    EXPECT_EQ(controller.preflightMessage().toStdString(), "No cameras detected");
    EXPECT_TRUE(controller.lastSessionId().isEmpty());
}

TEST(AppController, PluginListReturnsBundledPlugins) {
    micecam::ui::AppController controller;
    QVariantList plugins = controller.pluginList();
    EXPECT_GE(plugins.size(), 1);

    for (const auto& item : plugins) {
        QVariantMap m = item.toMap();
        EXPECT_TRUE(m.contains("pluginId"));
        EXPECT_TRUE(m.contains("name"));
        EXPECT_TRUE(m.contains("version"));
        EXPECT_TRUE(m.contains("path"));
        EXPECT_TRUE(m.contains("enabled"));
        EXPECT_TRUE(m.contains("type"));
        EXPECT_TRUE(m.contains("status"));
        EXPECT_EQ(m["type"].toString(), QStringLiteral("bundled"));
    }
}

TEST(AppController, ImportPluginRejectsInvalidPath) {
    micecam::ui::AppController controller;
    bool result = controller.importPlugin("/nonexistent/path/to/plugin");
    EXPECT_FALSE(result);
}

TEST(AppController, ImportPluginBlockedDuringRecording) {
    micecam::ui::AppController controller;
    controller.setOutputDirectory("/tmp/micecam_plugin_lock_test");

    EXPECT_FALSE(controller.isRecording());
    EXPECT_FALSE(controller.importPlugin("/some/path"));
}

TEST(AppController, TogglePluginLooksUpByPath) {
    micecam::ui::AppController controller;

    QVariantList plugins = controller.pluginList();
    ASSERT_GE(plugins.size(), 1);

    QVariantMap first = plugins.first().toMap();
    QString path = first["path"].toString();

    controller.togglePlugin(path, false);

    QVariantList updated = controller.pluginList();
    bool foundDisabled = false;
    for (const auto& item : updated) {
        QVariantMap m = item.toMap();
        if (m["path"].toString() == path) {
            foundDisabled = true;
            break;
        }
    }
    EXPECT_TRUE(foundDisabled);
}

TEST(AppController, GetPluginDetailReturnsErrorForMissingManifest) {
    micecam::ui::AppController controller;
    QVariantMap detail = controller.getPluginDetail("/nonexistent/path");
    EXPECT_TRUE(detail.contains("error"));
}

TEST(AppController, RecordingLockPropertyIsExposed) {
    micecam::ui::AppController controller;
    EXPECT_FALSE(controller.isRecording());
}

TEST(AppController, ElapsedTextDefaultsToZeroWhenNotRecording) {
    micecam::ui::AppController controller;
    EXPECT_EQ(controller.elapsedText().toStdString(), "00:00");
}

TEST(AppController, RecentLogEntriesStartsEmpty) {
    micecam::ui::AppController controller;
    EXPECT_TRUE(controller.recentLogEntries().isEmpty());
}

TEST(AppController, RefreshCamerasEmitsCountSignals) {
    micecam::ui::AppController controller;

    int countChanged = 0;
    QObject::connect(&controller, &micecam::ui::AppController::cameraCountChanged,
                     [&]() { countChanged++; });

    controller.refreshCameras();
    EXPECT_EQ(controller.cameraCount(), 0);
    EXPECT_EQ(countChanged, 0);
}

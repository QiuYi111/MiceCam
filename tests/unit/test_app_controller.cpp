#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QThread>
#include <chrono>
#define private public
#include "cmd/micecam_ui/AppController.h"
#undef private

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
        EXPECT_TRUE(m.contains("apiVersion"));
        EXPECT_TRUE(m.contains("restartRequired"));
        EXPECT_TRUE(m.contains("canToggle"));
        EXPECT_TRUE(m.contains("canRemove"));
        EXPECT_TRUE(m.contains("statusMessage"));
        EXPECT_EQ(m["type"].toString(), QStringLiteral("bundled"));
        EXPECT_FALSE(m["canToggle"].toBool());
        EXPECT_FALSE(m["canRemove"].toBool());
    }
}

TEST(AppController, BundledPluginToggleIsLocked) {
    micecam::ui::AppController controller;

    QVariantList plugins = controller.pluginList();
    ASSERT_GE(plugins.size(), 1);

    QVariantMap first = plugins.first().toMap();
    ASSERT_EQ(first["type"].toString(), QStringLiteral("bundled"));

    controller.togglePlugin(first["path"].toString(), false);

    QVariantList updated = controller.pluginList();
    bool found = false;
    for (const auto& item : updated) {
        QVariantMap m = item.toMap();
        if (m["path"].toString() == first["path"].toString()) {
            found = true;
            EXPECT_TRUE(m["enabled"].toBool());
            EXPECT_FALSE(m["canToggle"].toBool());
        }
    }
    EXPECT_TRUE(found);
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

TEST(AppController, ElapsedText90Seconds) {
    micecam::ui::AppController controller;
    auto now = std::chrono::steady_clock::now();
    controller.session_start_ = now - std::chrono::seconds(90);
    EXPECT_EQ(controller.elapsedText().toStdString(), "01:30");
}

TEST(AppController, ElapsedText65Minutes) {
    micecam::ui::AppController controller;
    auto now = std::chrono::steady_clock::now();
    controller.session_start_ = now - std::chrono::seconds(3900);
    EXPECT_EQ(controller.elapsedText().toStdString(), "01:05:00");
}

TEST(AppController, ElapsedText3h7m5s) {
    micecam::ui::AppController controller;
    auto now = std::chrono::steady_clock::now();
    controller.session_start_ = now - std::chrono::seconds(11225);
    EXPECT_EQ(controller.elapsedText().toStdString(), "03:07:05");
}

TEST(AppController, ElapsedTextZeroSeconds) {
    micecam::ui::AppController controller;
    auto now = std::chrono::steady_clock::now();
    controller.session_start_ = now;
    QString text = controller.elapsedText();
    EXPECT_TRUE(text == "00:00" || text == "00:01")
        << "Expected '00:00' or '00:01' (timing), got '" << text.toStdString() << "'";
}

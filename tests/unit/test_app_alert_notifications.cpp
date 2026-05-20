#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTimer>

#define private public
#include "cmd/micecam_ui/AppAlertModel.h"
#undef private

int main(int argc, char** argv) {
    if (!QCoreApplication::instance()) {
        static QCoreApplication app(argc, argv);
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

using namespace micecam::ui;

TEST(AppAlertNotifications, PushAlertAddsRowWithSeverity) {
    AppAlertModel model;
    model.pushAlert("Connection lost", "usb-cam-1", 1,
                     "alert_disconnect_1", false);

    ASSERT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.badgeCount(), 1);

    QModelIndex idx = model.index(0, 0);
    EXPECT_EQ(model.data(idx, AppAlertModel::SeverityRole).toInt(), 1);
    EXPECT_EQ(model.data(idx, AppAlertModel::TitleRole).toString(),
              "Connection lost");
    EXPECT_EQ(model.data(idx, AppAlertModel::SourceRole).toString(),
              "usb-cam-1");
}

TEST(AppAlertNotifications, PushAlertContainsMessagePluginNameRecoverable) {
    AppAlertModel model;
    model.pushAlert("Plugin crash detected — recovering...",
                     "micecam.ffmpeg", 1, "crash_ffmpeg", true);

    ASSERT_EQ(model.rowCount(), 1);
    QModelIndex idx = model.index(0, 0);
    EXPECT_EQ(model.data(idx, AppAlertModel::SeverityRole).toInt(), 1);
    EXPECT_EQ(model.data(idx, AppAlertModel::TitleRole).toString(),
              "Plugin crash detected — recovering...");
    EXPECT_EQ(model.data(idx, AppAlertModel::SourceRole).toString(),
              "micecam.ffmpeg");
}

TEST(AppAlertNotifications, AutoDismissRemovesAlert) {
    AppAlertModel model;
    model.pushAlert("Recovering...", "plugin_a", 1,
                     "crash_plugin_a", true);

    ASSERT_EQ(model.rowCount(), 1);
    model.dismissAlert("crash_plugin_a");
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.badgeCount(), 0);
}

TEST(AppAlertNotifications, CrashRecoverySuccessAutoDismissesAlert) {
    AppAlertModel model;
    model.pushAlert("Plugin crash detected — recovering...",
                     "micecam.oak", 1, "crash_oak", true);
    EXPECT_EQ(model.rowCount(), 1);

    model.dismissBySource("micecam.oak");
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(AppAlertNotifications, CrashRecoveryFailureEscalatesToRed) {
    AppAlertModel model;
    model.pushAlert("Plugin crash detected — recovering...",
                     "micecam.ffmpeg", 1, "crash_ffmpeg", true);
    EXPECT_EQ(model.rowCount(), 1);

    model.dismissBySource("micecam.ffmpeg");
    EXPECT_EQ(model.rowCount(), 0);

    model.pushAlert("Plugin recovery failed — micecam.ffmpeg",
                     "micecam.ffmpeg", 2, "crash_ffmpeg", false);
    ASSERT_EQ(model.rowCount(), 1);
    QModelIndex idx = model.index(0, 0);
    EXPECT_EQ(model.data(idx, AppAlertModel::SeverityRole).toInt(), 2);
}

TEST(AppAlertNotifications, DeviceDisconnectProducesWarningAlert) {
    AppAlertModel model;
    model.pushAlert("Device disconnected: OAK-D — check connection",
                     "OAK-D", 1, "disconnect_oak_stream_0", false);

    ASSERT_EQ(model.rowCount(), 1);
    QModelIndex idx = model.index(0, 0);
    EXPECT_EQ(model.data(idx, AppAlertModel::SeverityRole).toInt(), 1);
    EXPECT_EQ(model.data(idx, AppAlertModel::SourceRole).toString(), "OAK-D");
}

TEST(AppAlertNotifications, MultipleAlertsTrackAll) {
    AppAlertModel model;
    model.pushAlert("Alert A", "src-a", 1, "id_a", false);
    model.pushAlert("Alert B", "src-b", 2, "id_b", false);
    model.pushAlert("Alert C", "src-c", 1, "id_c", false);

    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_EQ(model.badgeCount(), 3);

    model.dismissAlert("id_b");
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.badgeCount(), 2);

    model.dismissBySource("src-a");
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.badgeCount(), 1);

    model.dismissAlert("id_c");
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.badgeCount(), 0);
}

TEST(AppAlertNotifications, ClearRemovesAllAlerts) {
    AppAlertModel model;
    model.pushAlert("Alert 1", "src-1", 1, "id_1", false);
    model.pushAlert("Alert 2", "src-2", 2, "id_2", false);

    EXPECT_EQ(model.rowCount(), 2);
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.badgeCount(), 0);
}

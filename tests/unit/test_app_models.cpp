#include <gtest/gtest.h>
#include <QCoreApplication>
#include "cmd/micecam_ui/AppCameraModel.h"
#include "cmd/micecam_ui/AppAlertModel.h"
#include "cmd/micecam_ui/CameraSourceModel.h"

static int s_argc = 0;
static QCoreApplication* s_app = nullptr;

int main(int argc, char** argv) {
    s_argc = argc;
    if (!QCoreApplication::instance()) {
        s_app = new QCoreApplication(argc, argv);
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(AppCameraModel, LoadsRowsFromBackendSnapshot) {
    micecam::ui::AppCameraModel model;
    micecam::ui::CameraRow row;
    row.cameraId = "mock_cam_0";
    row.name = "CAM_A";
    row.fps = 30.0;
    row.dropCount = 0;
    row.recording = false;
    row.status = 0;
    row.resolutionLabels = {"1920 x 1080", "1280 x 720"};
    row.framerateLabels = {"15 fps", "30 fps"};
    row.formatLabels = {"rgb24"};

    model.replaceRows({row});
    ASSERT_EQ(model.rowCount(), 1);
    const auto index = model.index(0, 0);
    EXPECT_EQ(model.data(index, micecam::ui::AppCameraModel::NameRole).toString(), "CAM_A");
    EXPECT_EQ(model.data(index, micecam::ui::AppCameraModel::IsRecordingRole).toBool(), false);
    EXPECT_EQ(model.data(index, micecam::ui::AppCameraModel::ResolutionOptionsRole).toStringList().size(), 2);
}

TEST(AppAlertModel, LoadsAlertsAndTracksBadgeCount) {
    micecam::ui::AppAlertModel model;
    micecam::ui::AlertRow row;
    row.severity = 1;
    row.title = "High drop rate";
    row.source = "CAM_A";
    row.relativeTime = "now";

    model.replaceRows({row});
    ASSERT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.badgeCount(), 1);
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.badgeCount(), 0);
}

TEST(CameraSourceModel, ExposesReleaseUiSourceRoles) {
    micecam::ui::CameraSourceModel model;

    micecam::domain::PluginSource source;
    source.source_id = "micecam.ffmpeg";
    source.source_name = "FFmpeg Official";
    source.source_type = micecam::domain::PluginSourceType::BUNDLED;
    source.plugin_version = "2.4.1";
    source.plugin_api_version = 2;
    source.enabled = true;
    source.diagnostics_state = micecam::domain::PluginDiagnosticsState::OK;
    source.diagnostics_message = "all systems nominal";
    source.restart_required = true;

    micecam::domain::PluginDeviceInfo device;
    device.device_id = "usb-1";
    device.display_name = "USB Microscope";
    device.plugin_id = "micecam.ffmpeg";
    device.status = "available";
    device.supports_h264 = true;
    device.max_width = 1920;
    device.max_height = 1080;
    device.max_framerate = 30.0;

    model.populateFromSources({source}, {device});

    ASSERT_EQ(model.rowCount(), 1);
    const auto index = model.index(0, 0);
    EXPECT_EQ(model.data(index, micecam::ui::CameraSourceModel::SourceIdRole).toString(), "micecam.ffmpeg");
    EXPECT_EQ(model.data(index, micecam::ui::CameraSourceModel::SourceNameRole).toString(), "FFmpeg Official");
    EXPECT_EQ(model.data(index, micecam::ui::CameraSourceModel::PluginVersionRole).toString(), "2.4.1");
    EXPECT_EQ(model.data(index, micecam::ui::CameraSourceModel::PluginApiVersionRole).toInt(), 2);
    EXPECT_EQ(model.data(index, micecam::ui::CameraSourceModel::DeviceCountRole).toInt(), 1);
    EXPECT_EQ(model.data(index, micecam::ui::CameraSourceModel::AvailableDeviceCountRole).toInt(), 1);
    EXPECT_TRUE(model.data(index, micecam::ui::CameraSourceModel::RestartRequiredRole).toBool());
    EXPECT_EQ(model.data(index, micecam::ui::CameraSourceModel::DiagnosticsMessageRole).toString(), "all systems nominal");
}

TEST(CameraSourceModel, OrdersActiveSourcesBeforeUnavailableSources) {
    micecam::ui::CameraSourceModel model;

    micecam::domain::PluginSource disabled;
    disabled.source_id = "linked.lab";
    disabled.source_name = "Lab Adapter";
    disabled.source_type = micecam::domain::PluginSourceType::LINKED;
    disabled.enabled = false;
    disabled.diagnostics_state = micecam::domain::PluginDiagnosticsState::DISABLED;

    micecam::domain::PluginSource healthy;
    healthy.source_id = "micecam.ffmpeg";
    healthy.source_name = "FFmpeg Official";
    healthy.source_type = micecam::domain::PluginSourceType::BUNDLED;
    healthy.enabled = true;
    healthy.diagnostics_state = micecam::domain::PluginDiagnosticsState::OK;

    micecam::domain::PluginDeviceInfo device;
    device.device_id = "usb-1";
    device.display_name = "USB Microscope";
    device.plugin_id = "micecam.ffmpeg";
    device.status = "available";

    model.populateFromSources({disabled, healthy}, {device});

    ASSERT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.data(model.index(0, 0), micecam::ui::CameraSourceModel::SourceIdRole).toString(), "micecam.ffmpeg");
    EXPECT_EQ(model.data(model.index(1, 0), micecam::ui::CameraSourceModel::SourceIdRole).toString(), "linked.lab");
}

TEST(CameraSourceModel, DeviceLookupReturnsUiMetadata) {
    micecam::ui::CameraSourceModel model;

    micecam::domain::PluginSource source;
    source.source_id = "micecam.oak";
    source.source_name = "OAK-D Official";
    source.diagnostics_state = micecam::domain::PluginDiagnosticsState::ERROR;
    source.diagnostics_message = "SDK missing";

    micecam::domain::PluginDeviceInfo device;
    device.device_id = "oak-1";
    device.display_name = "OAK-D";
    device.plugin_id = "micecam.oak";
    device.status = "unavailable";
    device.supports_raw = true;
    device.exclusive_resource_id = "oak-usb-1";
    device.has_diagnostics = true;
    device.diagnostics_code = "SDK_MISSING";
    device.diagnostics_message = "DepthAI SDK not found";

    model.populateFromSources({source}, {device});
    QVariantMap row = model.getDeviceAt(0, 0);

    EXPECT_EQ(row["deviceId"].toString(), "oak-1");
    EXPECT_EQ(row["displayName"].toString(), "OAK-D");
    EXPECT_EQ(row["sourceId"].toString(), "micecam.oak");
    EXPECT_EQ(row["sourceName"].toString(), "OAK-D Official");
    EXPECT_EQ(row["exclusiveResourceId"].toString(), "oak-usb-1");
    EXPECT_EQ(row["diagnosticsCode"].toString(), "SDK_MISSING");
    EXPECT_EQ(row["diagnosticsMessage"].toString(), "DepthAI SDK not found");
}

#include <gtest/gtest.h>
#include <QCoreApplication>
#include "cmd/micecam_ui/AppCameraModel.h"
#include "cmd/micecam_ui/AppAlertModel.h"

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

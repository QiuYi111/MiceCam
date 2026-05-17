#include "cmd/micecam_ui/CameraInventoryModel.h"
#include "cmd/micecam_ui/RecordingSetup.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QTemporaryDir>

namespace {

CaptureDeviceDescriptor makeDevice() {
    CaptureDeviceDescriptor device;
    device.deviceId = "ffmpeg:0";
    device.backendId = "ffmpeg";
    device.displayName = "USB Camera";
    device.deviceIndex = 0;
    device.available = true;
    device.supportedResolutions = {"1920x1080", "1280x720"};
    device.supportedFps = {30, 60};
    return device;
}

}  // namespace

TEST(NativeAppPreflightTest, RejectsWhenInventoryIsEmpty) {
    const RecordingPreflightResult result = validateRecordingSetup(
        {},
        -1,
        "session",
        "recordings",
        "1920x1080",
        30.0,
        false
    );

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.message, "Connect a camera to begin.");
}

TEST(NativeAppPreflightTest, RejectsUnsupportedSettings) {
    const RecordingPreflightResult result = validateRecordingSetup(
        {makeDevice()},
        0,
        "session",
        "recordings",
        "640x480",
        24.0,
        false
    );

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.message, "Choose a supported resolution for the selected camera.");
}

TEST(NativeAppPreflightTest, SanitizesSessionNameAndAcceptsWritableOutput) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    const RecordingPreflightResult result = validateRecordingSetup(
        {makeDevice()},
        0,
        " trial:/capture 01 ",
        tempDir.path(),
        "1920x1080",
        30.0,
        false
    );

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.sanitizedSessionName, "trial_capture_01");
    EXPECT_EQ(result.message, "Ready to record.");
}

TEST(CameraInventoryModelTest, ExposesTypedRoles) {
    CameraInventoryModel model;
    model.setDevices({makeDevice()});

    ASSERT_EQ(model.rowCount(), 1);
    const QModelIndex index = model.index(0, 0);
    EXPECT_EQ(model.data(index, CameraInventoryModel::NameRole).toString(), "USB Camera");
    EXPECT_EQ(model.data(index, CameraInventoryModel::BackendIdRole).toString(), "ffmpeg");
    EXPECT_EQ(model.data(index, CameraInventoryModel::AvailableRole).toBool(), true);
    EXPECT_EQ(model.data(index, CameraInventoryModel::ResolutionsRole).toStringList().size(), 2);
}

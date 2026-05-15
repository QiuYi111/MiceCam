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

TEST(AppController, MockModeDiscoversUiReadyCameras) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.refreshCameras();
    ASSERT_NE(controller.cameraModel(), nullptr);
    EXPECT_GE(controller.cameraModel()->rowCount(), 1);
    EXPECT_EQ(controller.cameraCountText().toStdString(), "5 cameras");
    EXPECT_FALSE(controller.isRecording());
    EXPECT_TRUE(controller.canStartRecording());
    EXPECT_EQ(controller.cameraCount(), 5);
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Start");
}

TEST(AppController, StartAndStopRecordingUpdatesState) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.setOutputDirectory("/tmp/micecam_app_controller");
    controller.refreshCameras();
    ASSERT_EQ(controller.cameraCount(), 5);
    EXPECT_TRUE(controller.canStartRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Start");

    ASSERT_TRUE(controller.startRecording());
    EXPECT_TRUE(controller.isRecording());
    EXPECT_FALSE(controller.canStartRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Stop");
    EXPECT_FALSE(controller.startRecording());
    EXPECT_TRUE(controller.isRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Stop");

    controller.stopRecording();
    EXPECT_FALSE(controller.isRecording());
    EXPECT_TRUE(controller.canStartRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Start");
    EXPECT_FALSE(controller.lastSessionId().isEmpty());
}

TEST(AppController, ProductionModeWithoutRegisteredBackendsShowsEmptyIdleState) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::Production);
    controller.refreshCameras();

    EXPECT_EQ(controller.cameraCount(), 0);
    ASSERT_NE(controller.cameraModel(), nullptr);
    EXPECT_EQ(controller.cameraModel()->rowCount(), 0);
    EXPECT_EQ(controller.cameraCountText().toStdString(), "0 cameras");
    EXPECT_FALSE(controller.isRecording());
    EXPECT_FALSE(controller.canStartRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "No Device");
    EXPECT_EQ(controller.preflightMessage().toStdString(), "No cameras detected");
    EXPECT_TRUE(controller.lastSessionId().isEmpty());
}

TEST(AppController, StartRecordingWithoutCamerasReportsPreflightFailure) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::Production);
    controller.refreshCameras();

    EXPECT_FALSE(controller.startRecording());
    EXPECT_EQ(controller.cameraCount(), 0);
    EXPECT_FALSE(controller.isRecording());
    EXPECT_FALSE(controller.canStartRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "No Device");
    EXPECT_EQ(controller.preflightMessage().toStdString(), "No cameras detected");
    EXPECT_TRUE(controller.lastSessionId().isEmpty());
}

TEST(AppController, RecordingPumpUpdatesFrameCounters) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.setOutputDirectory("/tmp/micecam_app_controller_pump");
    controller.refreshCameras();

    ASSERT_TRUE(controller.startRecording());
    QThread::msleep(600);
    controller.stopRecording();

    EXPECT_NE(controller.totalFramesText().toStdString(), "0");
    EXPECT_FALSE(controller.bytesWrittenText().isEmpty());
}

TEST(AppController, ElapsedTextReflectsRecordingDuration) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.setOutputDirectory("/tmp/micecam_app_controller_elapsed");
    controller.refreshCameras();

    EXPECT_EQ(controller.elapsedText().toStdString(), "00:00");

    ASSERT_TRUE(controller.startRecording());
    QThread::msleep(1500);

    EXPECT_NE(controller.elapsedText().toStdString(), "00:00");

    controller.stopRecording();
    // After stop, elapsed should reset
    EXPECT_EQ(controller.elapsedText().toStdString(), "00:00");
}

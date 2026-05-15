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
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Record");
}

TEST(AppController, StartAndStopRecordingUpdatesState) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.setOutputDirectory("/tmp/micecam_app_controller");
    controller.refreshCameras();
    ASSERT_TRUE(controller.startRecording());
    EXPECT_TRUE(controller.isRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Stop");
    controller.stopRecording();
    EXPECT_FALSE(controller.isRecording());
    EXPECT_EQ(controller.recordButtonText().toStdString(), "Record");
    EXPECT_FALSE(controller.lastSessionId().isEmpty());
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

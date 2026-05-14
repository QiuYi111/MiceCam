#include "cmd/micecam_ui/RecordingSupervisorService.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>

#include <type_traits>
#include <utility>

namespace {

template <typename T, typename = void>
struct HasDeviceNameMember : std::false_type {};

template <typename T>
struct HasDeviceNameMember<T, std::void_t<decltype(std::declval<T>().deviceName)>> : std::true_type {};

class FakeRecordingRuntime final : public micecam_ui::IRecordingRuntime {
public:
    bool launch(const QString& workerProgram, QString* errorMessage) override {
        Q_UNUSED(workerProgram);
        launched = launchResult;
        if (!launchResult && errorMessage) {
            *errorMessage = launchError;
        }
        return launchResult;
    }

    bool startSession(const micecam_ui::RecordingStartRequest& request, QString* errorMessage) override {
        lastRequest = request;
        startCalls += 1;
        if (!startResult && errorMessage) {
            *errorMessage = startError;
        }
        return startResult;
    }

    bool stopSession(QString* errorMessage) override {
        stopCalls += 1;
        if (!stopResult && errorMessage) {
            *errorMessage = stopError;
        }
        return stopResult;
    }

    bool requestShutdown(QString* errorMessage) override {
        shutdownCalls += 1;
        if (!shutdownResult && errorMessage) {
            *errorMessage = shutdownError;
        }
        return shutdownResult;
    }

    void setObserver(micecam_ui::IRecordingRuntimeObserver* nextObserver) override {
        observer = nextObserver;
    }

    void emitState(const micecam_ui::RuntimeStatus& status) {
        observer->onRuntimeStatus(status);
    }

    void emitActivity(const micecam_ui::ActivityEvent& event) {
        observer->onRuntimeActivity(event);
    }

    void emitExited(bool expected, const QString& reason) {
        observer->onRuntimeExited(expected, reason);
    }

    void emitPreview(const QByteArray& jpegBytes) {
        observer->onRuntimePreview(jpegBytes);
    }

    bool launchResult = true;
    QString launchError = "worker failed to launch";
    bool startResult = true;
    QString startError = "worker rejected start";
    bool stopResult = true;
    QString stopError = "worker rejected stop";
    bool shutdownResult = true;
    QString shutdownError = "worker rejected shutdown";
    bool launched = false;
    int startCalls = 0;
    int stopCalls = 0;
    int shutdownCalls = 0;
    micecam_ui::RecordingStartRequest lastRequest;
    micecam_ui::IRecordingRuntimeObserver* observer = nullptr;
};

class RecordingSupervisorServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime = std::make_unique<FakeRecordingRuntime>();
        runtimePtr = runtime.get();
        service = std::make_unique<micecam_ui::RecordingSupervisorService>(std::move(runtime));
    }

    micecam_ui::RecordingStartRequest makeRequest(bool autoDecode = true) {
        QTemporaryDir dir;
        EXPECT_TRUE(dir.isValid());

        micecam_ui::RecordingStartRequest request;
        request.backendId = "ffmpeg";
        request.deviceIndex = 0;
        request.deviceName = "HD USB Camera";
        request.sessionName = "session_001";
        request.outputDir = dir.path();
        request.resolution = QSize(1920, 1080);
        request.fps = 30.0;
        request.autoDecode = autoDecode;
        return request;
    }

    std::unique_ptr<FakeRecordingRuntime> runtime;
    FakeRecordingRuntime* runtimePtr = nullptr;
    std::unique_ptr<micecam_ui::RecordingSupervisorService> service;
};

TEST_F(RecordingSupervisorServiceTest, SuccessfulFlowTransitionsToCompletedAfterDecode) {
    const auto request = makeRequest(true);

    ASSERT_TRUE(service->startRecording(request, "/tmp/micecam_ui_worker"));
    EXPECT_EQ(service->state(), "launching_worker");

    runtimePtr->emitState({.state = "worker_ready"});
    runtimePtr->emitState({.state = "recording", .detail = "Recording now", .capturedFrames = 42, .throughputMbps = 128.0});

    EXPECT_TRUE(service->isRecording());
    EXPECT_EQ(service->state(), "recording");
    EXPECT_EQ(service->capturedFrames(), 42u);

    ASSERT_TRUE(service->stopRecording());
    EXPECT_EQ(service->state(), "stopping");

    runtimePtr->emitState({.state = "decoding", .detail = "Preparing export", .decodeProgress = 35.0});
    EXPECT_TRUE(service->isDecoding());
    EXPECT_EQ(service->decodeProgress(), 35.0);

    runtimePtr->emitState({.state = "completed", .detail = "Export ready", .resolvedExportPath = "/tmp/export"});
    EXPECT_EQ(service->state(), "completed");
    EXPECT_FALSE(service->isRecording());
    EXPECT_FALSE(service->isDecoding());
    EXPECT_EQ(service->resolvedExportPath(), "/tmp/export");
    EXPECT_GE(service->activityModel()->rowCount(), 1);
}

TEST_F(RecordingSupervisorServiceTest, ReportsWorkerLaunchFailure) {
    runtimePtr->launchResult = false;
    runtimePtr->launchError = "worker executable missing";

    EXPECT_FALSE(service->startRecording(makeRequest(), "/tmp/missing_worker"));
    EXPECT_EQ(service->state(), "error");
    EXPECT_EQ(service->lastErrorMessage(), "worker executable missing");
}

TEST_F(RecordingSupervisorServiceTest, WorkerReadyStateAllowsRecordingToStart) {
    ASSERT_TRUE(service->startRecording(makeRequest(), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({.state = "worker_ready", .detail = "Worker ready."});

    EXPECT_EQ(service->state(), "preflight");
    EXPECT_TRUE(service->canRequestStart());
}

TEST_F(RecordingSupervisorServiceTest, UnexpectedWorkerExitKeepsRecoveryInNonStartableStateUntilRuntimeConfirmsResume) {
    ASSERT_TRUE(service->startRecording(makeRequest(), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({.state = "recording", .detail = "Recording now"});
    runtimePtr->emitExited(false, "worker crashed");

    EXPECT_EQ(service->state(), "recovering");
    EXPECT_EQ(service->statusDetail(), "worker crashed Recovery resumed the session with a new worker.");
    EXPECT_EQ(runtimePtr->startCalls, 2);
    EXPECT_FALSE(service->canRequestStart());
    EXPECT_FALSE(service->isRecording());
    ASSERT_GE(service->activityModel()->rowCount(), 1);
}

TEST_F(RecordingSupervisorServiceTest, DecodeFailureKeepsAppControllable) {
    ASSERT_TRUE(service->startRecording(makeRequest(true), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({.state = "recording"});
    ASSERT_TRUE(service->stopRecording());
    runtimePtr->emitState({.state = "decoding", .detail = "Preparing export", .decodeProgress = 12.0});
    runtimePtr->emitState({.state = "error", .detail = "decode failed"});

    EXPECT_EQ(service->state(), "error");
    EXPECT_EQ(service->lastErrorMessage(), "decode failed");
    EXPECT_TRUE(service->canRequestStart());
}

TEST_F(RecordingSupervisorServiceTest, CloseDuringDecodeRequestsRuntimeShutdown) {
    ASSERT_TRUE(service->startRecording(makeRequest(true), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({.state = "recording"});
    ASSERT_TRUE(service->stopRecording());
    runtimePtr->emitState({.state = "decoding", .detail = "Preparing export"});

    EXPECT_FALSE(service->canCloseSafely());
    EXPECT_TRUE(service->prepareForClose());
    EXPECT_EQ(runtimePtr->shutdownCalls, 1);
}

TEST_F(RecordingSupervisorServiceTest, ExpectedExitAfterShutdownMakesCloseSafe) {
    ASSERT_TRUE(service->startRecording(makeRequest(true), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({.state = "recording"});
    ASSERT_TRUE(service->stopRecording());
    runtimePtr->emitState({.state = "decoding", .detail = "Preparing export"});

    EXPECT_FALSE(service->canCloseSafely());
    ASSERT_TRUE(service->prepareForClose());

    runtimePtr->emitExited(true, QString());

    EXPECT_TRUE(service->canCloseSafely());
    EXPECT_EQ(service->state(), "idle");
    EXPECT_FALSE(service->isDecoding());
}

TEST_F(RecordingSupervisorServiceTest, TracksPreviewPolicyFromRuntimeStatus) {
    ASSERT_TRUE(service->startRecording(makeRequest(), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({
        .state = "recording",
        .detail = "Recording now",
        .previewAvailable = false,
        .previewMode = "disabled",
        .previewDetail = "Preview IPC is disabled in the current worker-process slice."
    });

    EXPECT_FALSE(service->previewAvailable());
    EXPECT_EQ(service->previewMode(), "disabled");
    EXPECT_EQ(service->previewDetail(), "Preview IPC is disabled in the current worker-process slice.");
}

TEST_F(RecordingSupervisorServiceTest, EmitsPreviewSignalWhenRuntimePublishesFrame) {
    QByteArray lastFrame;
    QObject::connect(
        service.get(),
        &micecam_ui::RecordingSupervisorService::previewFrameReady,
        [&lastFrame](const QByteArray& jpegBytes) { lastFrame = jpegBytes; }
    );

    runtimePtr->emitPreview("jpeg-bytes");
    EXPECT_EQ(lastFrame, QByteArray("jpeg-bytes"));
}

TEST_F(RecordingSupervisorServiceTest, WorkerRestartFailureLeavesSupervisorInError) {
    ASSERT_TRUE(service->startRecording(makeRequest(), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({.state = "recording", .detail = "Recording now"});
    runtimePtr->launchResult = false;
    runtimePtr->launchError = "worker relaunch failed";
    runtimePtr->emitExited(false, "worker crashed");

    EXPECT_EQ(service->state(), "error");
    EXPECT_EQ(service->lastErrorMessage(), "worker crashed Recovery failed: worker relaunch failed");
    EXPECT_FALSE(service->canRequestStart());
}

TEST_F(RecordingSupervisorServiceTest, ResumeFailureFallsBackToRestartedReadyState) {
    ASSERT_TRUE(service->startRecording(makeRequest(), "/tmp/micecam_ui_worker"));

    runtimePtr->emitState({.state = "recording", .detail = "Recording now"});
    runtimePtr->startResult = false;
    runtimePtr->startError = "device re-acquire failed";
    runtimePtr->emitExited(false, "worker crashed");

    EXPECT_EQ(service->state(), "error");
    EXPECT_EQ(service->lastErrorMessage(), "worker crashed Worker restarted but session resume failed: device re-acquire failed");
    EXPECT_TRUE(service->canRequestStart());
    EXPECT_EQ(runtimePtr->startCalls, 2);
}

TEST_F(RecordingSupervisorServiceTest, RecordingStartRequestShouldCarryUsbCameraDisplayName) {
    EXPECT_TRUE(HasDeviceNameMember<micecam_ui::RecordingStartRequest>::value);
}

TEST_F(RecordingSupervisorServiceTest, ForwardsUsbCameraDisplayNameToRuntime) {
    const auto request = makeRequest();

    ASSERT_TRUE(service->startRecording(request, "/tmp/micecam_ui_worker"));

    EXPECT_EQ(runtimePtr->lastRequest.deviceName, "HD USB Camera");
}

}  // namespace

#include "cmd/micecam_ui/PipelineController.h"
#include "cmd/micecam_ui/RecordingSupervisorService.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTimer>

namespace {

class FakeRecordingRuntime final : public micecam_ui::IRecordingRuntime {
public:
    bool launch(const QString& workerProgram, QString* errorMessage) override {
        Q_UNUSED(workerProgram);
        Q_UNUSED(errorMessage);
        return true;
    }

    bool startSession(const micecam_ui::RecordingStartRequest& request, QString* errorMessage) override {
        Q_UNUSED(request);
        Q_UNUSED(errorMessage);
        return true;
    }

    bool stopSession(QString* errorMessage) override {
        Q_UNUSED(errorMessage);
        return true;
    }

    bool requestShutdown(QString* errorMessage) override {
        Q_UNUSED(errorMessage);
        shutdownCalls += 1;
        return true;
    }

    bool forceShutdown(QString* errorMessage) override {
        Q_UNUSED(errorMessage);
        forceShutdownCalls += 1;
        return true;
    }

    void setObserver(micecam_ui::IRecordingRuntimeObserver* nextObserver) override {
        observer = nextObserver;
    }

    void emitState(const micecam_ui::RuntimeStatus& status) {
        observer->onRuntimeStatus(status);
    }

    void emitExited(bool expected, const QString& reason = QString()) {
        observer->onRuntimeExited(expected, reason);
    }

    int shutdownCalls = 0;
    int forceShutdownCalls = 0;
    micecam_ui::IRecordingRuntimeObserver* observer = nullptr;
};

micecam_ui::RecordingStartRequest makeRequest() {
    QTemporaryDir dir;
    EXPECT_TRUE(dir.isValid());

    micecam_ui::RecordingStartRequest request;
    request.backendId = "ffmpeg";
    request.deviceIndex = 0;
    request.sessionName = "session_001";
    request.outputDir = dir.path();
    request.resolution = QSize(1920, 1080);
    request.fps = 30.0;
    request.autoDecode = true;
    return request;
}

TEST(PipelineControllerTest, BusyCloseCompletesAfterExpectedWorkerExit) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);

    auto runtime = std::make_unique<FakeRecordingRuntime>();
    auto* runtimePtr = runtime.get();
    auto supervisor = std::make_unique<micecam_ui::RecordingSupervisorService>(std::move(runtime));
    auto* supervisorPtr = supervisor.get();
    PipelineController controller(std::move(supervisor));

    ASSERT_TRUE(supervisorPtr->startRecording(makeRequest(), "/tmp/micecam_ui_worker"));
    runtimePtr->emitState({.state = "recording"});
    ASSERT_TRUE(supervisorPtr->stopRecording());
    runtimePtr->emitState({.state = "decoding", .detail = "Preparing export"});

    bool quitRequested = false;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&quitRequested]() { quitRequested = true; });

    EXPECT_FALSE(controller.requestAppClose());
    EXPECT_EQ(runtimePtr->shutdownCalls, 1);
    EXPECT_FALSE(quitRequested);

    bool fallbackTriggered = false;
    QTimer::singleShot(0, [&runtimePtr]() { runtimePtr->emitExited(true); });
    QTimer::singleShot(250, [&app, &fallbackTriggered]() {
        fallbackTriggered = true;
        app.exit(99);
    });
    app.exec();

    EXPECT_TRUE(quitRequested);
    EXPECT_FALSE(fallbackTriggered);
}

TEST(PipelineControllerTest, PrefersExistingSessionPathWhenDecodeOutputIsMissing) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString sessionPath = QDir(dir.path()).filePath("session_001");
    ASSERT_TRUE(QDir().mkpath(sessionPath));

    auto runtime = std::make_unique<FakeRecordingRuntime>();
    auto* runtimePtr = runtime.get();
    auto supervisor = std::make_unique<micecam_ui::RecordingSupervisorService>(std::move(runtime));
    auto* supervisorPtr = supervisor.get();
    PipelineController controller(std::move(supervisor));

    auto request = makeRequest();
    request.outputDir = dir.path();
    request.sessionName = "session_001";

    ASSERT_TRUE(supervisorPtr->startRecording(request, "/tmp/micecam_ui_worker"));
    runtimePtr->emitState({.state = "recording"});
    ASSERT_TRUE(supervisorPtr->stopRecording());
    runtimePtr->emitState({.state = "decoding", .detail = "Preparing export"});
    runtimePtr->emitState({
        .state = "error",
        .detail = "decode failed",
        .resolvedSessionPath = sessionPath,
        .resolvedExportPath = QDir(dir.path()).filePath("session_001_decoded")
    });

    EXPECT_EQ(controller.getPreferredOutputPath(), sessionPath);
}

TEST(PipelineControllerTest, PreviewToggleDefaultsOnAndCanBeDisabled) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);

    auto runtime = std::make_unique<FakeRecordingRuntime>();
    auto supervisor = std::make_unique<micecam_ui::RecordingSupervisorService>(std::move(runtime));
    PipelineController controller(std::move(supervisor));

    EXPECT_TRUE(controller.previewEnabled());
    controller.setPreviewEnabled(false);
    EXPECT_FALSE(controller.previewEnabled());
}

TEST(PipelineControllerTest, ShutdownForExitForceStopsWorker) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);

    auto runtime = std::make_unique<FakeRecordingRuntime>();
    auto* runtimePtr = runtime.get();
    auto supervisor = std::make_unique<micecam_ui::RecordingSupervisorService>(std::move(runtime));
    auto* supervisorPtr = supervisor.get();
    PipelineController controller(std::move(supervisor));

    ASSERT_TRUE(supervisorPtr->startRecording(makeRequest(), "/tmp/micecam_ui_worker"));

    controller.shutdownForExit();

    EXPECT_EQ(runtimePtr->forceShutdownCalls, 1);
}

}  // namespace

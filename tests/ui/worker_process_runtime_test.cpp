#include "cmd/micecam_ui/WorkerProcessRuntime.h"

#include <gtest/gtest.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

class RuntimeObserver final : public micecam_ui::IRecordingRuntimeObserver {
public:
    void onRuntimeStatus(const micecam_ui::RuntimeStatus& status) override { statuses.push_back(status); }
    void onRuntimeActivity(const micecam_ui::ActivityEvent& event) override { activities.push_back(event); }
    void onRuntimeExited(bool wasExpected, const QString& exitReason) override {
        expected = wasExpected;
        reason = exitReason;
        exitCalls += 1;
    }
    void onRuntimePreview(const QByteArray& jpegBytes) override { previewFrames.push_back(jpegBytes); }

    QList<micecam_ui::RuntimeStatus> statuses;
    QList<micecam_ui::ActivityEvent> activities;
    QList<QByteArray> previewFrames;
    bool expected = true;
    QString reason;
    int exitCalls = 0;
};

TEST(WorkerProcessRuntimeTest, LaunchFailsWhenChildNeverPublishesHello) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);

    const QString trueProgram = QStandardPaths::findExecutable("true");
    if (trueProgram.isEmpty()) {
        GTEST_SKIP() << "System 'true' executable is unavailable on this platform.";
    }

    RuntimeObserver observer;
    micecam_ui::WorkerProcessRuntime runtime;
    runtime.setObserver(&observer);

    QString errorMessage;
    EXPECT_FALSE(runtime.launch(trueProgram, &errorMessage));
    EXPECT_FALSE(errorMessage.isEmpty());
    EXPECT_GE(observer.exitCalls, 1);
    EXPECT_FALSE(observer.expected);
}

TEST(WorkerProcessRuntimeTest, StartSessionReturnsAfterSendingCommandToWorker) {
    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app(argc, argv);

    const QString helperPath = QDir(QCoreApplication::applicationDirPath()).filePath("worker_protocol_hang_helper");
    ASSERT_TRUE(QFileInfo::exists(helperPath)) << helperPath.toStdString();

    RuntimeObserver observer;
    micecam_ui::WorkerProcessRuntime runtime;
    runtime.setObserver(&observer);

    QString errorMessage;
    ASSERT_TRUE(runtime.launch(helperPath, &errorMessage)) << errorMessage.toStdString();

    micecam_ui::RecordingStartRequest request;
    request.backendId = "ffmpeg";
    request.deviceIndex = 0;
    request.sessionName = "session_001";
    request.outputDir = "/tmp";
    request.resolution = QSize(1280, 720);
    request.fps = 30.0;
    request.autoDecode = false;

    EXPECT_TRUE(runtime.startSession(request, &errorMessage)) << errorMessage.toStdString();
    EXPECT_TRUE(runtime.forceShutdown(&errorMessage)) << errorMessage.toStdString();
    EXPECT_EQ(observer.exitCalls, 0);
}

}  // namespace

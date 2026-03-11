#pragma once

#include "RecordingSupervisorService.h"

#include <QObject>

class QProcess;

namespace micecam_ui {

class WorkerProcessRuntime : public QObject, public IRecordingRuntime {
    Q_OBJECT

public:
    explicit WorkerProcessRuntime(QObject* parent = nullptr);
    ~WorkerProcessRuntime() override;

    bool launch(const QString& workerProgram, QString* errorMessage) override;
    bool startSession(const RecordingStartRequest& request, QString* errorMessage) override;
    bool stopSession(QString* errorMessage) override;
    bool requestShutdown(QString* errorMessage) override;
    void setObserver(IRecordingRuntimeObserver* observer) override;

private:
    bool sendCommand(const QVariantMap& command, QString* errorMessage);
    void handleStdout();
    void handleStderr();
    void handleFinished(int exitCode);

    QProcess* m_process = nullptr;
    IRecordingRuntimeObserver* m_observer = nullptr;
    QByteArray m_stdoutBuffer;
    bool m_shutdownRequested = false;
};

}  // namespace micecam_ui

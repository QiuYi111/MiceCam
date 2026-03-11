#pragma once

#include "ActivityEventModel.h"

#include <QObject>
#include <QSize>
#include <QString>
#include <memory>

namespace micecam_ui {

struct RecordingStartRequest {
    QString backendId;
    int deviceIndex = -1;
    QString sessionName;
    QString outputDir;
    QSize resolution;
    double fps = 0.0;
    bool autoDecode = true;
};

struct RuntimeStatus {
    QString state;
    QString detail;
    uint64_t capturedFrames = 0;
    uint64_t droppedFrames = 0;
    double throughputMbps = 0.0;
    double currentFps = 0.0;
    double decodeProgress = -1.0;
    QString resolvedSessionPath;
    QString resolvedExportPath;
};

class IRecordingRuntimeObserver {
public:
    virtual ~IRecordingRuntimeObserver() = default;
    virtual void onRuntimeStatus(const RuntimeStatus& status) = 0;
    virtual void onRuntimeActivity(const ActivityEvent& event) = 0;
    virtual void onRuntimeExited(bool expected, const QString& reason) = 0;
};

class IRecordingRuntime {
public:
    virtual ~IRecordingRuntime() = default;
    virtual bool launch(const QString& workerProgram, QString* errorMessage) = 0;
    virtual bool startSession(const RecordingStartRequest& request, QString* errorMessage) = 0;
    virtual bool stopSession(QString* errorMessage) = 0;
    virtual bool requestShutdown(QString* errorMessage) = 0;
    virtual void setObserver(IRecordingRuntimeObserver* observer) = 0;
};

class RecordingSupervisorService : public QObject, public IRecordingRuntimeObserver {
    Q_OBJECT

public:
    explicit RecordingSupervisorService(std::unique_ptr<IRecordingRuntime> runtime, QObject* parent = nullptr);
    ~RecordingSupervisorService() override = default;

    bool startRecording(const RecordingStartRequest& request, const QString& workerProgram);
    bool stopRecording();
    bool prepareForClose();

    QString state() const;
    QString statusDetail() const;
    QString lastErrorMessage() const;
    QString resolvedSessionPath() const;
    QString resolvedExportPath() const;
    bool isRecording() const;
    bool isDecoding() const;
    bool canRequestStart() const;
    bool canCloseSafely() const;
    uint64_t capturedFrames() const;
    uint64_t droppedFrames() const;
    double throughputMbps() const;
    double currentFps() const;
    double decodeProgress() const;
    ActivityEventModel* activityModel() const;

    void onRuntimeStatus(const RuntimeStatus& status) override;
    void onRuntimeActivity(const ActivityEvent& event) override;
    void onRuntimeExited(bool expected, const QString& reason) override;

signals:
    void supervisorChanged();

private:
    void setState(const QString& state, const QString& detail = QString());
    void setError(const QString& errorMessage);
    void appendActivity(const QString& severity, const QString& category, const QString& message, const QString& path = QString());

    std::unique_ptr<IRecordingRuntime> m_runtime;
    ActivityEventModel* m_activityModel = nullptr;
    RecordingStartRequest m_lastRequest;
    QString m_state = "idle";
    QString m_statusDetail = "Choose a camera and destination to prepare the next capture.";
    QString m_lastErrorMessage;
    QString m_resolvedSessionPath;
    QString m_resolvedExportPath;
    bool m_runtimeLaunched = false;
    bool m_isRecording = false;
    bool m_isDecoding = false;
    uint64_t m_capturedFrames = 0;
    uint64_t m_droppedFrames = 0;
    double m_throughputMbps = 0.0;
    double m_currentFps = 0.0;
    double m_decodeProgress = 0.0;
};

}  // namespace micecam_ui

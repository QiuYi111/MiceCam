#include "RecordingSupervisorService.h"

#include <QDir>

namespace micecam_ui {

RecordingSupervisorService::RecordingSupervisorService(
    std::unique_ptr<IRecordingRuntime> runtime,
    QObject* parent
) : QObject(parent),
    m_runtime(std::move(runtime)),
    m_activityModel(new ActivityEventModel(this)) {
    m_runtime->setObserver(this);
}

bool RecordingSupervisorService::startRecording(
    const RecordingStartRequest& request,
    const QString& workerProgram
) {
    if (!canRequestStart()) {
        setError("Capture is busy. Wait for the current recording or export to finish.");
        return false;
    }

    m_lastRequest = request;
    m_resolvedSessionPath = QDir(request.outputDir).filePath(request.sessionName);
    m_resolvedExportPath = request.autoDecode
        ? QDir(request.outputDir).filePath(request.sessionName + "_decoded")
        : m_resolvedSessionPath;
    m_lastErrorMessage.clear();
    m_decodeProgress = 0.0;

    QString errorMessage;
    if (!m_runtimeLaunched) {
        if (!m_runtime->launch(workerProgram, &errorMessage)) {
            setError(errorMessage);
            return false;
        }
        m_runtimeLaunched = true;
    }

    if (!m_runtime->startSession(request, &errorMessage)) {
        setError(errorMessage);
        return false;
    }

    appendActivity("info", "session", "Recording worker launching.", m_resolvedSessionPath);
    setState("launching_worker", "Launching recording worker.");
    return true;
}

bool RecordingSupervisorService::stopRecording() {
    if (!m_isRecording && m_state != "recording") {
        return false;
    }

    QString errorMessage;
    if (!m_runtime->stopSession(&errorMessage)) {
        setError(errorMessage);
        return false;
    }

    setState("stopping", "Stopping recording safely.");
    appendActivity("session", "session", "Stop requested.", m_resolvedSessionPath);
    return true;
}

bool RecordingSupervisorService::prepareForClose() {
    if (canCloseSafely()) {
        return true;
    }

    QString errorMessage;
    if (!m_runtime->requestShutdown(&errorMessage)) {
        setError(errorMessage);
        return false;
    }

    appendActivity("warning", "system", "Application close requested while runtime is busy.");
    return true;
}

QString RecordingSupervisorService::state() const { return m_state; }
QString RecordingSupervisorService::statusDetail() const { return m_statusDetail; }
QString RecordingSupervisorService::lastErrorMessage() const { return m_lastErrorMessage; }
QString RecordingSupervisorService::resolvedSessionPath() const { return m_resolvedSessionPath; }
QString RecordingSupervisorService::resolvedExportPath() const { return m_resolvedExportPath; }
bool RecordingSupervisorService::isRecording() const { return m_isRecording; }
bool RecordingSupervisorService::isDecoding() const { return m_isDecoding; }
bool RecordingSupervisorService::canRequestStart() const {
    return m_state == "idle" || m_state == "completed" || m_state == "error";
}
bool RecordingSupervisorService::canCloseSafely() const {
    return !(m_state == "launching_worker" || m_state == "recording" ||
             m_state == "recovering" || m_state == "stopping" || m_state == "decoding");
}
uint64_t RecordingSupervisorService::capturedFrames() const { return m_capturedFrames; }
uint64_t RecordingSupervisorService::droppedFrames() const { return m_droppedFrames; }
double RecordingSupervisorService::throughputMbps() const { return m_throughputMbps; }
double RecordingSupervisorService::currentFps() const { return m_currentFps; }
double RecordingSupervisorService::decodeProgress() const { return m_decodeProgress; }
ActivityEventModel* RecordingSupervisorService::activityModel() const { return m_activityModel; }

void RecordingSupervisorService::onRuntimeStatus(const RuntimeStatus& status) {
    if (!status.resolvedSessionPath.isEmpty()) {
        m_resolvedSessionPath = status.resolvedSessionPath;
    }
    if (!status.resolvedExportPath.isEmpty()) {
        m_resolvedExportPath = status.resolvedExportPath;
    }
    if (status.decodeProgress >= 0.0) {
        m_decodeProgress = status.decodeProgress;
    }

    m_capturedFrames = status.capturedFrames;
    m_droppedFrames = status.droppedFrames;
    m_throughputMbps = status.throughputMbps;
    m_currentFps = status.currentFps;

    if (status.state == "worker_ready") {
        setState("preflight", status.detail.isEmpty() ? "Worker ready." : status.detail);
        appendActivity("info", "system", "Recording worker ready.");
    } else if (status.state == "recording") {
        m_isRecording = true;
        m_isDecoding = false;
        setState("recording", status.detail.isEmpty() ? "Recording now. Watch preview and capture health." : status.detail);
    } else if (status.state == "stopping") {
        m_isRecording = false;
        m_isDecoding = false;
        setState("stopping", status.detail.isEmpty() ? "Stopping recording safely." : status.detail);
    } else if (status.state == "decoding") {
        m_isRecording = false;
        m_isDecoding = true;
        setState("decoding", status.detail.isEmpty() ? "Preparing export." : status.detail);
    } else if (status.state == "completed") {
        m_isRecording = false;
        m_isDecoding = false;
        setState("completed", status.detail.isEmpty() ? "Export is ready. Review the files or start another session." : status.detail);
        appendActivity("info", "export", "Session completed.", m_resolvedExportPath);
    } else if (status.state == "error") {
        setError(status.detail.isEmpty() ? "Recording worker reported an error." : status.detail);
    }

    emit supervisorChanged();
}

void RecordingSupervisorService::onRuntimeActivity(const ActivityEvent& event) {
    m_activityModel->appendEvent(event);
    emit supervisorChanged();
}

void RecordingSupervisorService::onRuntimeExited(bool expected, const QString& reason) {
    if (expected) {
        appendActivity("info", "system", "Recording worker exited.");
        return;
    }

    setState("recovering", "Recording worker exited unexpectedly.");
    setError(reason.isEmpty() ? "Recording worker exited unexpectedly." : reason);
}

void RecordingSupervisorService::setState(const QString& state, const QString& detail) {
    m_state = state;
    if (!detail.isNull()) {
        m_statusDetail = detail;
    }
    emit supervisorChanged();
}

void RecordingSupervisorService::setError(const QString& errorMessage) {
    m_isRecording = false;
    m_isDecoding = false;
    m_lastErrorMessage = errorMessage;
    appendActivity("error", "system", errorMessage, m_resolvedSessionPath);
    setState("error", errorMessage);
}

void RecordingSupervisorService::appendActivity(
    const QString& severity,
    const QString& category,
    const QString& message,
    const QString& path
) {
    ActivityEvent event;
    event.severity = severity;
    event.category = category;
    event.message = message;
    event.relatedPath = path;
    m_activityModel->appendEvent(event);
}

}  // namespace micecam_ui

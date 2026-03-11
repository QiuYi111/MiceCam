#include "RecordingSupervisorService.h"

#include <QDir>

namespace micecam_ui {

namespace {

constexpr int kStartupTimeoutMs = 45000;

}  // namespace

RecordingSupervisorService::RecordingSupervisorService(
    std::unique_ptr<IRecordingRuntime> runtime,
    QObject* parent
) : QObject(parent),
    m_runtime(std::move(runtime)),
    m_activityModel(new ActivityEventModel(this)) {
    m_runtime->setObserver(this);
    m_startupWatchdog = new QTimer(this);
    m_startupWatchdog->setSingleShot(true);
    connect(m_startupWatchdog, &QTimer::timeout, this, &RecordingSupervisorService::handleStartupTimeout);
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
    m_workerProgram = workerProgram;

    QString errorMessage;
    if (!m_runtimeLaunched || !m_runtimeHealthy) {
        if (!m_runtime->launch(workerProgram, &errorMessage)) {
            setError(errorMessage);
            return false;
        }
        m_runtimeLaunched = true;
        m_runtimeHealthy = true;
    }

    if (!m_runtime->startSession(request, &errorMessage)) {
        setError(errorMessage);
        return false;
    }

    m_startPending = true;
    m_startupWatchdog->start(kStartupTimeoutMs);
    m_shouldAttemptResume = true;
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

    cancelStartupWatchdog();
    m_shouldAttemptResume = false;
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

    cancelStartupWatchdog();
    appendActivity("warning", "system", "Application close requested while runtime is busy.");
    return true;
}

void RecordingSupervisorService::shutdownForExit() {
    cancelStartupWatchdog();
    m_shouldAttemptResume = false;
    m_startPending = false;

    if (!m_runtimeLaunched) {
        return;
    }

    QString errorMessage;
    if (!m_runtime->forceShutdown(&errorMessage) && !errorMessage.isEmpty()) {
        appendActivity("warning", "system", errorMessage);
    }

    m_runtimeLaunched = false;
    m_runtimeHealthy = false;
    m_isRecording = false;
    m_isDecoding = false;
    m_previewAvailable = false;
}

QString RecordingSupervisorService::state() const { return m_state; }
QString RecordingSupervisorService::statusDetail() const { return m_statusDetail; }
QString RecordingSupervisorService::lastErrorMessage() const { return m_lastErrorMessage; }
QString RecordingSupervisorService::resolvedSessionPath() const { return m_resolvedSessionPath; }
QString RecordingSupervisorService::resolvedExportPath() const { return m_resolvedExportPath; }
bool RecordingSupervisorService::previewAvailable() const { return m_previewAvailable; }
QString RecordingSupervisorService::previewMode() const { return m_previewMode; }
QString RecordingSupervisorService::previewDetail() const { return m_previewDetail; }
bool RecordingSupervisorService::isRecording() const { return m_isRecording; }
bool RecordingSupervisorService::isDecoding() const { return m_isDecoding; }
bool RecordingSupervisorService::canRequestStart() const {
    return m_runtimeHealthy && (
        m_state == "idle" ||
        m_state == "preflight" ||
        m_state == "completed" ||
        m_state == "error");
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
    m_previewAvailable = status.previewAvailable;
    if (!status.previewMode.isEmpty()) {
        m_previewMode = status.previewMode;
    }
    if (!status.previewDetail.isEmpty()) {
        m_previewDetail = status.previewDetail;
    }
    if (status.decodeProgress >= 0.0) {
        m_decodeProgress = status.decodeProgress;
    }

    m_capturedFrames = status.capturedFrames;
    m_droppedFrames = status.droppedFrames;
    m_throughputMbps = status.throughputMbps;
    m_currentFps = status.currentFps;

    if (status.state == "worker_ready") {
        m_runtimeHealthy = true;
        if (!m_startPending) {
            setState("preflight", status.detail.isEmpty() ? "Worker ready." : status.detail);
        }
        appendActivity("info", "system", "Recording worker ready.");
    } else if (status.state == "recording") {
        cancelStartupWatchdog();
        m_isRecording = true;
        m_isDecoding = false;
        if (m_previewDetail.isEmpty()) {
            m_previewDetail = "Preview is running within the configured budget.";
        }
        setState("recording", status.detail.isEmpty() ? "Recording now. Watch preview and capture health." : status.detail);
    } else if (status.state == "stopping") {
        cancelStartupWatchdog();
        m_isRecording = false;
        m_isDecoding = false;
        setState("stopping", status.detail.isEmpty() ? "Stopping recording safely." : status.detail);
    } else if (status.state == "decoding") {
        cancelStartupWatchdog();
        m_isRecording = false;
        m_isDecoding = true;
        setState("decoding", status.detail.isEmpty() ? "Preparing export." : status.detail);
    } else if (status.state == "completed") {
        cancelStartupWatchdog();
        m_isRecording = false;
        m_isDecoding = false;
        m_shouldAttemptResume = false;
        setState("completed", status.detail.isEmpty() ? "Export is ready. Review the files or start another session." : status.detail);
        appendActivity("info", "export", "Session completed.", m_resolvedExportPath);
    } else if (status.state == "error") {
        cancelStartupWatchdog();
        m_shouldAttemptResume = false;
        setError(status.detail.isEmpty() ? "Recording worker reported an error." : status.detail);
    }

    emit supervisorChanged();
}

void RecordingSupervisorService::onRuntimeActivity(const ActivityEvent& event) {
    m_activityModel->appendEvent(event);
    emit supervisorChanged();
}

void RecordingSupervisorService::onRuntimePreview(const QByteArray& jpegBytes) {
    emit previewFrameReady(jpegBytes);
}

void RecordingSupervisorService::onRuntimeExited(bool expected, const QString& reason) {
    cancelStartupWatchdog();
    m_runtimeLaunched = false;

    if (expected) {
        m_runtimeHealthy = false;
        m_isRecording = false;
        m_isDecoding = false;
        m_shouldAttemptResume = false;
        m_previewAvailable = false;
        appendActivity("info", "system", "Recording worker exited.");
        setState("idle", "Recording worker shut down.");
        return;
    }

    const bool wasRecording = m_isRecording || m_state == "launching_worker";
    const bool wasBusy = wasRecording || m_isDecoding || m_state == "stopping";
    m_runtimeHealthy = false;
    m_isRecording = false;
    m_isDecoding = false;
    m_previewAvailable = false;
    setState("recovering", "Recording worker exited unexpectedly.");
    appendActivity("warning", "system", "Recording worker crashed. Attempting restart.");

    QString relaunchError;
    if (relaunchWorker(&relaunchError)) {
        const QString crashReason = reason.isEmpty()
            ? QStringLiteral("Recording worker exited unexpectedly.")
            : reason;

        if (wasRecording && m_shouldAttemptResume) {
            QString resumeError;
            if (resumeLastSession(&resumeError)) {
                const QString message = QStringLiteral("%1 Recovery resumed the session with a new worker.")
                    .arg(crashReason);
                appendActivity("warning", "system", message, m_resolvedSessionPath);
                setState("recovering", message);
            } else {
                const QString message = QStringLiteral("%1 Worker restarted but session resume failed: %2")
                    .arg(crashReason, resumeError.isEmpty() ? QStringLiteral("resume failed") : resumeError);
                setError(message);
            }
            return;
        }

        const QString message = QStringLiteral("%1 Worker restarted and app is ready for another session.")
            .arg(crashReason);
        if (wasBusy) {
            setError(message);
        } else {
            appendActivity("warning", "system", message);
            setState("idle", "Worker restarted. Ready for the next session.");
        }
        return;
    }

    const QString message = QStringLiteral("%1 Recovery failed: %2")
        .arg(reason.isEmpty() ? QStringLiteral("Recording worker exited unexpectedly.") : reason,
             relaunchError.isEmpty() ? QStringLiteral("worker relaunch failed") : relaunchError);
    setFatalError(message);
}

void RecordingSupervisorService::setState(const QString& state, const QString& detail) {
    m_state = state;
    if (!detail.isNull()) {
        m_statusDetail = detail;
    }
    emit supervisorChanged();
}

void RecordingSupervisorService::setError(const QString& errorMessage) {
    cancelStartupWatchdog();
    m_isRecording = false;
    m_isDecoding = false;
    m_lastErrorMessage = errorMessage;
    m_previewAvailable = false;
    appendActivity("error", "system", errorMessage, m_resolvedSessionPath);
    setState("error", errorMessage);
}

void RecordingSupervisorService::setFatalError(const QString& errorMessage) {
    m_runtimeHealthy = false;
    setError(errorMessage);
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

bool RecordingSupervisorService::relaunchWorker(QString* errorMessage) {
    if (m_workerProgram.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "worker path is not configured";
        }
        return false;
    }

    if (!m_runtime->launch(m_workerProgram, errorMessage)) {
        return false;
    }

    m_runtimeLaunched = true;
    m_runtimeHealthy = true;
    appendActivity("info", "system", "Recording worker restarted successfully.");
    return true;
}

bool RecordingSupervisorService::resumeLastSession(QString* errorMessage) {
    appendActivity("warning", "session", "Attempting device re-acquire and session resume.", m_resolvedSessionPath);
    if (!m_runtime->startSession(m_lastRequest, errorMessage)) {
        m_shouldAttemptResume = false;
        return false;
    }

    m_startPending = true;
    m_startupWatchdog->start(kStartupTimeoutMs);
    m_shouldAttemptResume = true;
    appendActivity("info", "session", "Session resume request accepted by restarted worker.", m_resolvedSessionPath);
    return true;
}

void RecordingSupervisorService::cancelStartupWatchdog() {
    m_startPending = false;
    if (m_startupWatchdog->isActive()) {
        m_startupWatchdog->stop();
    }
}

void RecordingSupervisorService::handleStartupTimeout() {
    m_startPending = false;
    m_shouldAttemptResume = false;

    QString shutdownError;
    if (!m_runtime->forceShutdown(&shutdownError) && !shutdownError.isEmpty()) {
        appendActivity("warning", "system", shutdownError);
    }

    m_runtimeLaunched = false;
    m_runtimeHealthy = true;

    const QString message = QStringLiteral(
        "Camera initialization timed out after %1 seconds. Check camera permission, device availability, and whether another app is using the camera."
    ).arg(kStartupTimeoutMs / 1000);
    setError(message);
}

}  // namespace micecam_ui

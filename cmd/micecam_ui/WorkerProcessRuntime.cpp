#include "WorkerProcessRuntime.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QVariantMap>

namespace micecam_ui {

namespace {

QString readJsonString(const QJsonObject& object, const char* key) {
    return object.value(QLatin1String(key)).toString();
}

}  // namespace

WorkerProcessRuntime::WorkerProcessRuntime(QObject* parent) : QObject(parent) {}

WorkerProcessRuntime::~WorkerProcessRuntime() {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool WorkerProcessRuntime::launch(const QString& workerProgram, QString* errorMessage) {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        return true;
    }

    delete m_process;
    m_process = new QProcess(this);
    m_stdoutBuffer.clear();
    m_shutdownRequested = false;

    connect(m_process, &QProcess::readyReadStandardOutput, this, &WorkerProcessRuntime::handleStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &WorkerProcessRuntime::handleStderr);
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
        handleFinished(exitCode);
    });

    m_process->setProgram(workerProgram);
    m_process->setArguments({QStringLiteral("--worker")});
    m_process->start();
    if (!m_process->waitForStarted(3000)) {
        if (errorMessage) {
            *errorMessage = m_process->errorString();
        }
        return false;
    }

    return true;
}

bool WorkerProcessRuntime::startSession(const RecordingStartRequest& request, QString* errorMessage) {
    QVariantMap command;
    command.insert("type", "start");
    command.insert("backendId", request.backendId);
    command.insert("deviceIndex", request.deviceIndex);
    command.insert("sessionName", request.sessionName);
    command.insert("outputDir", request.outputDir);
    command.insert("width", request.resolution.width());
    command.insert("height", request.resolution.height());
    command.insert("fps", request.fps);
    command.insert("autoDecode", request.autoDecode);
    return sendCommand(command, errorMessage);
}

bool WorkerProcessRuntime::stopSession(QString* errorMessage) {
    return sendCommand({{"type", "stop"}}, errorMessage);
}

bool WorkerProcessRuntime::requestShutdown(QString* errorMessage) {
    m_shutdownRequested = true;
    return sendCommand({{"type", "shutdown"}}, errorMessage);
}

void WorkerProcessRuntime::setObserver(IRecordingRuntimeObserver* observer) {
    m_observer = observer;
}

bool WorkerProcessRuntime::sendCommand(const QVariantMap& command, QString* errorMessage) {
    if (!m_process || m_process->state() != QProcess::Running) {
        if (errorMessage) {
            *errorMessage = "Recording worker is not running.";
        }
        return false;
    }

    const QByteArray payload = QJsonDocument::fromVariant(command).toJson(QJsonDocument::Compact) + '\n';
    if (m_process->write(payload) != payload.size() || !m_process->waitForBytesWritten(1000)) {
        if (errorMessage) {
            *errorMessage = "Failed to send command to recording worker.";
        }
        return false;
    }
    return true;
}

void WorkerProcessRuntime::handleStdout() {
    if (!m_observer || !m_process) {
        return;
    }

    m_stdoutBuffer.append(m_process->readAllStandardOutput());
    while (true) {
        const int newlineIndex = m_stdoutBuffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        const QByteArray line = m_stdoutBuffer.left(newlineIndex).trimmed();
        m_stdoutBuffer.remove(0, newlineIndex + 1);
        if (line.isEmpty()) {
            continue;
        }

        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject()) {
            continue;
        }

        const QJsonObject object = document.object();
        const QString type = readJsonString(object, "type");
        if (type == "activity") {
            ActivityEvent event;
            event.severity = readJsonString(object, "severity");
            event.category = readJsonString(object, "category");
            event.message = readJsonString(object, "message");
            event.relatedPath = readJsonString(object, "relatedPath");
            m_observer->onRuntimeActivity(event);
            continue;
        }

        if (type == "preview") {
            const QByteArray jpegBytes = QByteArray::fromBase64(
                readJsonString(object, "jpegBase64").toUtf8()
            );
            m_observer->onRuntimePreview(jpegBytes);
            continue;
        }

        if (type == "status" || type == "hello") {
            RuntimeStatus status;
            status.state = type == "hello" ? QStringLiteral("worker_ready") : readJsonString(object, "state");
            status.detail = readJsonString(object, "detail");
            status.capturedFrames = static_cast<uint64_t>(object.value("capturedFrames").toVariant().toULongLong());
            status.droppedFrames = static_cast<uint64_t>(object.value("droppedFrames").toVariant().toULongLong());
            status.throughputMbps = object.value("throughputMbps").toDouble();
            status.currentFps = object.value("currentFps").toDouble();
            status.decodeProgress = object.contains("decodeProgress") ? object.value("decodeProgress").toDouble() : -1.0;
            status.resolvedSessionPath = readJsonString(object, "resolvedSessionPath");
            status.resolvedExportPath = readJsonString(object, "resolvedExportPath");
            status.previewAvailable = object.value("previewAvailable").toBool(false);
            status.previewMode = readJsonString(object, "previewMode");
            status.previewDetail = readJsonString(object, "previewDetail");
            m_observer->onRuntimeStatus(status);
        }
    }
}

void WorkerProcessRuntime::handleStderr() {
    if (!m_observer || !m_process) {
        return;
    }

    const QString stderrText = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
    if (stderrText.isEmpty()) {
        return;
    }

    ActivityEvent event;
    event.severity = "warning";
    event.category = "system";
    event.message = stderrText;
    m_observer->onRuntimeActivity(event);
}

void WorkerProcessRuntime::handleFinished(int exitCode) {
    if (!m_observer) {
        return;
    }

    const bool expected = m_shutdownRequested && exitCode == 0;
    const QString reason = expected
        ? QString()
        : QStringLiteral("Recording worker exited with code %1.").arg(exitCode);
    m_observer->onRuntimeExited(expected, reason);
}

}  // namespace micecam_ui

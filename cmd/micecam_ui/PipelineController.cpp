#include "PipelineController.h"

#include "CameraInventoryModel.h"
#include "RecordingSetup.h"
#include "RecordingSupervisorService.h"
#include "VideoFrameProvider.h"
#include "WorkerProcessRuntime.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QMediaDevices>
#include <QSet>
#include <QUrl>

#ifdef WITH_OAK_CAMERA
#include <depthai/depthai.hpp>
#endif

#ifdef WITH_FFMPEG
#include "micecam/camera/ffmpeg_camera_backend.h"
#endif

namespace {

using micecam_ui::ActivityEventModel;
using micecam_ui::RecordingStartRequest;

CaptureDeviceDescriptor makeOakDevice(const QString& name, int index) {
    CaptureDeviceDescriptor device;
    device.deviceId = QStringLiteral("oak:%1").arg(index);
    device.backendId = "oak";
    device.displayName = name;
    device.deviceIndex = index;
    device.available = true;
    device.supportedResolutions = {"1280x800", "1280x720", "640x400"};
    device.supportedFps = {30, 60, 120};
    return device;
}

void populateQtCameraCapabilities(
    const QCameraDevice& camera,
    QStringList& resolutions,
    QList<int>& fpsValues
) {
    QSet<QString> uniqueResolutions;
    QSet<int> uniqueFpsValues;

    const auto formats = camera.videoFormats();
    for (const QCameraFormat& format : formats) {
        const QSize resolution = format.resolution();
        if (resolution.isValid()) {
            uniqueResolutions.insert(QStringLiteral("%1x%2").arg(resolution.width()).arg(resolution.height()));
        }

        const int minFps = qRound(format.minFrameRate());
        const int maxFps = qRound(format.maxFrameRate());
        if (minFps > 0) {
            uniqueFpsValues.insert(minFps);
        }
        if (maxFps > 0) {
            uniqueFpsValues.insert(maxFps);
        }
    }

    resolutions = uniqueResolutions.values();
    std::sort(resolutions.begin(), resolutions.end(), [](const QString& left, const QString& right) {
        const auto leftParts = left.split("x");
        const auto rightParts = right.split("x");
        if (leftParts.size() != 2 || rightParts.size() != 2) {
            return left < right;
        }

        const int leftArea = leftParts.at(0).toInt() * leftParts.at(1).toInt();
        const int rightArea = rightParts.at(0).toInt() * rightParts.at(1).toInt();
        if (leftArea == rightArea) {
            return left < right;
        }
        return leftArea > rightArea;
    });

    fpsValues = uniqueFpsValues.values();
    std::sort(fpsValues.begin(), fpsValues.end());
}

QList<CaptureDeviceDescriptor> enumerateDevices() {
    QList<CaptureDeviceDescriptor> devices;

#ifdef WITH_OAK_CAMERA
    try {
        const auto oakDevices = dai::DeviceBase::getAllAvailableDevices();
        for (int i = 0; i < static_cast<int>(oakDevices.size()); ++i) {
            const auto& info = oakDevices.at(i);
            const std::string name = info.name.empty() ? "Luxonis OAK (DepthAI)" : info.name;
            devices.push_back(makeOakDevice(QString::fromStdString(name), i));
        }
    } catch (const std::exception&) {
    }
#endif

#ifdef WITH_FFMPEG
    micecam::FFmpegCameraBackend ffmpegBackend;
    QStringList fallbackResolutions;
    for (const std::string& resolution : ffmpegBackend.get_supported_resolutions()) {
        fallbackResolutions.push_back(QString::fromStdString(resolution));
    }
    QList<int> fallbackFps;
    for (const int fps : ffmpegBackend.get_supported_fps()) {
        fallbackFps.push_back(fps);
    }

    const auto cameras = QMediaDevices::videoInputs();
    for (int i = 0; i < cameras.size(); ++i) {
        QStringList cameraResolutions;
        QList<int> cameraFps;
        populateQtCameraCapabilities(cameras.at(i), cameraResolutions, cameraFps);

        CaptureDeviceDescriptor device;
        device.deviceId = QStringLiteral("ffmpeg:%1").arg(i);
        device.backendId = "ffmpeg";
        device.displayName = cameras.at(i).description();
        device.deviceIndex = i;
        device.available = true;
        device.supportedResolutions = cameraResolutions.isEmpty() ? fallbackResolutions : cameraResolutions;
        device.supportedFps = cameraFps.isEmpty() ? fallbackFps : cameraFps;
        devices.push_back(device);
    }
#endif

    return devices;
}

}  // namespace

PipelineController::PipelineController(QObject* parent)
    : QObject(parent),
      m_cameraInventoryModel(new CameraInventoryModel(this)),
      m_mediaDevices(new QMediaDevices(this)),
      m_supervisor(std::make_unique<micecam_ui::RecordingSupervisorService>(
          std::make_unique<micecam_ui::WorkerProcessRuntime>())) {
    connect(m_mediaDevices, &QMediaDevices::videoInputsChanged, this, &PipelineController::refreshCameraInventory);
    connect(
        m_supervisor.get(),
        &micecam_ui::RecordingSupervisorService::supervisorChanged,
        this,
        &PipelineController::syncFromSupervisor
    );
    connect(
        m_supervisor->activityModel(),
        &QAbstractItemModel::rowsInserted,
        this,
        [this]() { rebuildLogMessages(); }
    );
    connect(
        m_supervisor->activityModel(),
        &QAbstractItemModel::modelReset,
        this,
        [this]() { rebuildLogMessages(); }
    );
    connect(
        m_supervisor.get(),
        &micecam_ui::RecordingSupervisorService::previewFrameReady,
        this,
        [this](const QByteArray& jpegBytes) {
            if (!m_videoProvider || jpegBytes.isEmpty()) {
                return;
            }

            QImage image;
            image.loadFromData(jpegBytes, "JPEG");
            if (!image.isNull()) {
                m_videoProvider->setPreviewImage(image);
            }
        }
    );

    m_sessionName = QStringLiteral("session_%1").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
    );
    refreshCameraInventory();
    refreshReadiness();
    rebuildLogMessages();
}

PipelineController::~PipelineController() {
    if (m_elapsedTimerId > 0) {
        killTimer(m_elapsedTimerId);
    }
    m_supervisor->prepareForClose();
}

bool PipelineController::isRecording() const { return m_supervisor->isRecording(); }
QString PipelineController::getSessionName() const { return m_sessionName; }
QString PipelineController::getOutputDir() const { return m_outputDir; }
double PipelineController::getCurrentFps() const { return m_supervisor->currentFps(); }
uint64_t PipelineController::getCapturedFrames() const { return m_supervisor->capturedFrames(); }
uint64_t PipelineController::getDroppedFrames() const { return m_supervisor->droppedFrames(); }
double PipelineController::getMbps() const { return m_supervisor->throughputMbps(); }
QString PipelineController::getFormat() const { return currentBackendId() == "oak" ? "MJPEG" : "MJPEG"; }
bool PipelineController::getAutoDecode() const { return m_autoDecode; }
QString PipelineController::getSessionState() const { return m_supervisor->state(); }
QString PipelineController::getStatusHeadline() const {
    const QString state = m_supervisor->state();
    if (state == "recording") return "Recording now";
    if (state == "decoding") return "Preparing export";
    if (state == "launching_worker") return "Launching worker";
    if (state == "completed") return "Ready for the next session";
    if (state == "error") return "Needs attention";
    return m_canStartRecording ? "Ready to record" : "Prepare the session";
}
QString PipelineController::getStatusDetail() const { return m_supervisor->statusDetail(); }
QObject* PipelineController::cameraInventoryModel() const { return m_cameraInventoryModel; }
QObject* PipelineController::activityModel() const { return m_supervisor->activityModel(); }
int PipelineController::selectedCameraIndex() const { return m_selectedCameraIndex; }
QStringList PipelineController::availableResolutions() const {
    const auto devices = m_cameraInventoryModel->devices();
    if (m_selectedCameraIndex < 0 || m_selectedCameraIndex >= devices.size()) {
        return {};
    }
    return devices.at(m_selectedCameraIndex).supportedResolutions;
}
QVariantList PipelineController::availableFps() const {
    QVariantList fpsValues;
    const auto devices = m_cameraInventoryModel->devices();
    if (m_selectedCameraIndex < 0 || m_selectedCameraIndex >= devices.size()) {
        return fpsValues;
    }
    for (const int fps : devices.at(m_selectedCameraIndex).supportedFps) {
        fpsValues.push_back(fps);
    }
    return fpsValues;
}
QString PipelineController::selectedResolution() const { return m_selectedResolution; }
double PipelineController::requestedFps() const { return m_requestedFps; }
bool PipelineController::canStartRecording() const { return m_canStartRecording; }
QString PipelineController::readinessMessage() const { return m_readinessMessage; }
QString PipelineController::sanitizedSessionName() const { return m_sanitizedSessionName; }
QString PipelineController::getLastErrorMessage() const { return m_supervisor->lastErrorMessage(); }
QString PipelineController::getLastSessionName() const { return m_lastSessionName; }
QString PipelineController::getDecodedOutputDir() const { return m_supervisor->resolvedExportPath(); }
int PipelineController::getElapsedSeconds() const {
    return (isRecording() && m_recordingTimer.isValid())
        ? static_cast<int>(m_recordingTimer.elapsed() / 1000)
        : 0;
}
QString PipelineController::getRecordingDurationText() const {
    const int totalSeconds = getElapsedSeconds();
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
bool PipelineController::isDecoding() const { return m_supervisor->isDecoding(); }
bool PipelineController::hasAvailableCamera() const { return m_cameraInventoryModel->rowCount() > 0; }
bool PipelineController::hasDroppedFramesWarning() const { return m_supervisor->droppedFrames() > 0; }
QString PipelineController::getResolvedSessionPath() const { return m_supervisor->resolvedSessionPath(); }
QString PipelineController::getResolvedExportPath() const { return m_supervisor->resolvedExportPath(); }
bool PipelineController::previewAvailable() const { return m_supervisor->previewAvailable(); }
QString PipelineController::previewMode() const { return m_supervisor->previewMode(); }
QString PipelineController::previewDetail() const { return m_supervisor->previewDetail(); }
QStringList PipelineController::logMessages() const { return m_logMessages; }
double PipelineController::getDecodeProgress() const { return m_supervisor->decodeProgress(); }

void PipelineController::setSessionName(const QString& name) {
    const QString sanitized = sanitizeSessionName(name);
    if (m_sessionName != sanitized) {
        m_sessionName = sanitized;
        emit sessionNameChanged(m_sessionName);
    }
    refreshReadiness();
}

void PipelineController::setOutputDir(const QString& dir) {
    const QString cleaned = QDir::cleanPath(dir.trimmed());
    if (m_outputDir != cleaned) {
        m_outputDir = cleaned;
        emit outputDirChanged(m_outputDir);
    }
    refreshReadiness();
}

void PipelineController::setAutoDecode(bool enable) {
    if (m_autoDecode != enable) {
        m_autoDecode = enable;
        emit autoDecodeChanged(m_autoDecode);
    }
}

void PipelineController::setSelectedCameraIndex(int index) {
    if (index == m_selectedCameraIndex) {
        return;
    }

    m_selectedCameraIndex = index;
    const QStringList resolutions = availableResolutions();
    if (!resolutions.contains(m_selectedResolution)) {
        m_selectedResolution = resolutions.isEmpty() ? QString() : resolutions.front();
        emit captureConfigChanged();
    }

    const QVariantList fpsValues = availableFps();
    bool fpsStillSupported = false;
    for (const QVariant& value : fpsValues) {
        if (value.toInt() == static_cast<int>(m_requestedFps)) {
            fpsStillSupported = true;
            break;
        }
    }
    if (!fpsStillSupported) {
        m_requestedFps = fpsValues.isEmpty() ? 0.0 : fpsValues.front().toDouble();
        emit captureConfigChanged();
    }

    emit selectedCameraChanged();
    refreshReadiness();
}

void PipelineController::setSelectedResolution(const QString& resolution) {
    if (m_selectedResolution == resolution) {
        return;
    }
    m_selectedResolution = resolution;
    emit captureConfigChanged();
    refreshReadiness();
}

void PipelineController::setRequestedFps(double fps) {
    if (qFuzzyCompare(m_requestedFps, fps)) {
        return;
    }
    m_requestedFps = fps;
    emit captureConfigChanged();
    refreshReadiness();
}

void PipelineController::refreshCameraInventory() {
    const QList<CaptureDeviceDescriptor> devices = enumerateDevices();
    m_cameraInventoryModel->setDevices(devices);

    int nextIndex = m_selectedCameraIndex;
    if (devices.isEmpty()) {
        nextIndex = -1;
    } else if (nextIndex < 0 || nextIndex >= devices.size()) {
        nextIndex = 0;
    }

    m_selectedCameraIndex = nextIndex;
    const QStringList resolutions = availableResolutions();
    if (!resolutions.contains(m_selectedResolution)) {
        m_selectedResolution = resolutions.isEmpty() ? QString() : resolutions.front();
    }

    const QVariantList fpsValues = availableFps();
    bool fpsStillSupported = false;
    for (const QVariant& value : fpsValues) {
        if (value.toInt() == static_cast<int>(m_requestedFps)) {
            fpsStillSupported = true;
            break;
        }
    }
    if (!fpsStillSupported) {
        m_requestedFps = fpsValues.isEmpty() ? 0.0 : fpsValues.front().toDouble();
    }

    emit cameraInventoryChanged();
    emit selectedCameraChanged();
    emit captureConfigChanged();
    refreshReadiness();
}

void PipelineController::startRecording() {
    if (!m_canStartRecording) {
        emit errorOccurred(m_readinessMessage);
        return;
    }

    const QStringList resolutionParts = m_selectedResolution.split("x");
    if (resolutionParts.size() != 2) {
        emit errorOccurred("The selected resolution is invalid.");
        return;
    }

    RecordingStartRequest request;
    request.backendId = currentBackendId();
    request.deviceIndex = currentDeviceIndex();
    request.sessionName = m_sanitizedSessionName;
    request.outputDir = m_outputDir;
    request.resolution = QSize(resolutionParts.at(0).toInt(), resolutionParts.at(1).toInt());
    request.fps = m_requestedFps;
    request.autoDecode = m_autoDecode;

    if (!m_supervisor->startRecording(request, QCoreApplication::applicationFilePath())) {
        syncFromSupervisor();
        emit errorOccurred(m_supervisor->lastErrorMessage());
        return;
    }

    m_lastSessionName = m_sanitizedSessionName;
    syncFromSupervisor();
    refreshReadiness();
}

void PipelineController::stopRecording() {
    if (!m_supervisor->stopRecording()) {
        syncFromSupervisor();
        return;
    }
    syncFromSupervisor();
}

bool PipelineController::openResolvedOutput() {
    const QString path = !getResolvedExportPath().isEmpty() ? getResolvedExportPath() : getResolvedSessionPath();
    if (path.isEmpty()) {
        return false;
    }
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool PipelineController::requestAppClose() {
    if (m_supervisor->canCloseSafely()) {
        return true;
    }
    m_supervisor->prepareForClose();
    syncFromSupervisor();
    return false;
}

void PipelineController::setVideoProvider(VideoFrameProvider* provider) {
    m_videoProvider = provider;
    Q_UNUSED(m_videoProvider);
}

void PipelineController::refreshReadiness() {
    const RecordingPreflightResult preflight = validateRecordingSetup(
        m_cameraInventoryModel->devices(),
        m_selectedCameraIndex,
        m_sessionName,
        m_outputDir,
        m_selectedResolution,
        m_requestedFps,
        !m_supervisor->canRequestStart()
    );

    bool changed = false;
    if (m_canStartRecording != preflight.ok) {
        m_canStartRecording = preflight.ok;
        changed = true;
    }
    if (m_readinessMessage != preflight.message) {
        m_readinessMessage = preflight.message;
        changed = true;
    }
    if (m_sanitizedSessionName != preflight.sanitizedSessionName) {
        m_sanitizedSessionName = preflight.sanitizedSessionName;
        changed = true;
    }

    if (changed) {
        emit readinessChanged();
    }
}

QString PipelineController::currentBackendId() const {
    const auto devices = m_cameraInventoryModel->devices();
    if (m_selectedCameraIndex < 0 || m_selectedCameraIndex >= devices.size()) {
        return {};
    }
    return devices.at(m_selectedCameraIndex).backendId;
}

int PipelineController::currentDeviceIndex() const {
    const auto devices = m_cameraInventoryModel->devices();
    if (m_selectedCameraIndex < 0 || m_selectedCameraIndex >= devices.size()) {
        return -1;
    }
    return devices.at(m_selectedCameraIndex).deviceIndex;
}

void PipelineController::syncFromSupervisor() {
    emit sessionStateChanged();
    emit errorStateChanged();
    emit sessionArchiveChanged();
    emit decodeStateChanged();
    emit decodeProgressChanged(m_supervisor->decodeProgress());
    emit statsUpdated();
    emit isRecordingChanged(m_supervisor->isRecording());

    if (m_supervisor->isRecording()) {
        if (!m_recordingTimer.isValid()) {
            m_recordingTimer.start();
        }
        if (m_elapsedTimerId == 0) {
            m_elapsedTimerId = startTimer(1000);
        }
    } else {
        if (m_elapsedTimerId > 0) {
            killTimer(m_elapsedTimerId);
            m_elapsedTimerId = 0;
        }
    }

    refreshReadiness();
}

void PipelineController::rebuildLogMessages() {
    m_logMessages.clear();
    auto* model = qobject_cast<ActivityEventModel*>(m_supervisor->activityModel());
    if (!model) {
        emit logMessagesChanged();
        return;
    }

    const int rows = model->rowCount();
    const int startRow = std::max(0, rows - 100);
    for (int row = startRow; row < rows; ++row) {
        const QModelIndex index = model->index(row, 0);
        const QString severity = model->data(index, ActivityEventModel::SeverityRole).toString().toUpper();
        const QString message = model->data(index, ActivityEventModel::MessageRole).toString();
        m_logMessages.push_back(QStringLiteral("[%1] %2").arg(severity, message));
    }
    emit logMessagesChanged();
}

void PipelineController::timerEvent(QTimerEvent* event) {
    if (event->timerId() != m_elapsedTimerId) {
        return;
    }
    emit statsUpdated();
}

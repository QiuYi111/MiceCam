#include "PipelineController.h"

#include "CameraInventoryModel.h"
#include "RecordingSetup.h"
#include "VideoFrameProvider.h"

#include <QDateTime>
#include <QDir>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QMetaObject>
#include <QTimerEvent>
#include <QtConcurrent/QtConcurrent>

#include "micecam/camera/camera_backend.h"
#include "micecam/pipeline/decoder.h"
#include "micecam/pipeline/ingestion_pipeline.h"

#ifdef WITH_OAK_CAMERA
#include "micecam/camera/oak_camera_backend.h"
#endif

#ifdef WITH_FFMPEG
#include "micecam/camera/ffmpeg_camera_backend.h"
#endif

namespace {

QString pixelFormatToUiString(micecam::PixelFormat format) {
    switch (format) {
    case micecam::PixelFormat::MONO8:
        return "MONO8";
    case micecam::PixelFormat::RGB24:
        return "RGB24";
    case micecam::PixelFormat::UYVY422:
        return "UYVY422";
    case micecam::PixelFormat::MJPEG:
        return "MJPEG";
    default:
        return "Unknown";
    }
}

CaptureDeviceDescriptor makeOakDevice() {
    CaptureDeviceDescriptor device;
    device.deviceId = "oak:0";
    device.backendId = "oak";
    device.displayName = "Luxonis OAK (DepthAI)";
    device.deviceIndex = 0;
    device.available = true;
#ifdef WITH_OAK_CAMERA
    micecam::OAKCameraBackend backend;
    for (const std::string& resolution : backend.get_supported_resolutions()) {
        device.supportedResolutions.push_back(QString::fromStdString(resolution));
    }
    for (const int fps : backend.get_supported_fps()) {
        device.supportedFps.push_back(fps);
    }
#else
    device.supportedResolutions = {"1280x800", "1280x720", "640x400"};
    device.supportedFps = {30, 60, 120};
#endif
    return device;
}

QList<CaptureDeviceDescriptor> enumerateDevices() {
    QList<CaptureDeviceDescriptor> devices;

#ifdef WITH_OAK_CAMERA
    devices.push_back(makeOakDevice());
#endif

#ifdef WITH_FFMPEG
    micecam::FFmpegCameraBackend ffmpegBackend;
    QStringList ffmpegResolutions;
    for (const std::string& resolution : ffmpegBackend.get_supported_resolutions()) {
        ffmpegResolutions.push_back(QString::fromStdString(resolution));
    }
    QList<int> ffmpegFps;
    for (const int fps : ffmpegBackend.get_supported_fps()) {
        ffmpegFps.push_back(fps);
    }

    const auto cameras = QMediaDevices::videoInputs();
    for (int i = 0; i < cameras.size(); ++i) {
        CaptureDeviceDescriptor device;
        device.deviceId = QStringLiteral("ffmpeg:%1").arg(i);
        device.backendId = "ffmpeg";
        device.displayName = cameras.at(i).description();
        device.deviceIndex = i;
        device.available = true;
        device.supportedResolutions = ffmpegResolutions;
        device.supportedFps = ffmpegFps;
        devices.push_back(device);
    }
#endif

    return devices;
}

}  // namespace

PipelineController::PipelineController(QObject* parent)
    : QObject(parent),
      m_cameraInventoryModel(new CameraInventoryModel(this)) {
    m_decoder = std::make_unique<micecam::Decoder>();
    m_mediaDevices = new QMediaDevices(this);
    connect(m_mediaDevices, &QMediaDevices::videoInputsChanged, this, &PipelineController::refreshCameraInventory);
    m_sessionName = QStringLiteral("session_%1").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
    );
    refreshCameraInventory();
    refreshReadiness();
}

PipelineController::~PipelineController() {
    stopRecording();
}

bool PipelineController::isRecording() const { return m_isRecording; }
QString PipelineController::getSessionName() const { return m_sessionName; }
QString PipelineController::getOutputDir() const { return m_outputDir; }
double PipelineController::getCurrentFps() const { return m_currentFps; }
uint64_t PipelineController::getCapturedFrames() const { return m_capturedFrames; }
uint64_t PipelineController::getDroppedFrames() const { return m_droppedFrames; }
double PipelineController::getMbps() const { return m_mbps; }
QString PipelineController::getFormat() const { return m_format; }
bool PipelineController::getAutoDecode() const { return m_autoDecode; }
QString PipelineController::getSessionState() const { return m_sessionState; }
QString PipelineController::getStatusHeadline() const {
    if (m_sessionState == "recording") return "Recording now";
    if (m_sessionState == "decoding") return "Preparing export";
    if (m_sessionState == "completed") return "Ready for the next session";
    if (m_sessionState == "error") return "Needs attention";
    return m_canStartRecording ? "Ready to record" : "Prepare the session";
}
QString PipelineController::getStatusDetail() const { return m_statusDetail; }
QObject* PipelineController::cameraInventoryModel() const { return m_cameraInventoryModel; }
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
QString PipelineController::getLastErrorMessage() const { return m_lastErrorMessage; }
QString PipelineController::getLastSessionName() const { return m_lastSessionName; }
QString PipelineController::getDecodedOutputDir() const { return m_decodedOutputDir; }
int PipelineController::getElapsedSeconds() const {
    return (m_isRecording && m_recordingTimer.isValid())
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
bool PipelineController::isDecoding() const { return m_isDecoding; }
bool PipelineController::hasAvailableCamera() const { return m_cameraInventoryModel->rowCount() > 0; }
bool PipelineController::hasDroppedFramesWarning() const { return m_droppedFrames > 0; }
QString PipelineController::getResolvedSessionPath() const {
    const QString session = m_lastSessionName.isEmpty() ? m_sanitizedSessionName : m_lastSessionName;
    const QString outputDir = m_lastOutputDir.isEmpty() ? m_outputDir : m_lastOutputDir;
    return QDir(outputDir).filePath(session);
}
QString PipelineController::getResolvedExportPath() const {
    if (!m_decodedOutputDir.isEmpty()) {
        return m_decodedOutputDir;
    }
    const QString session = m_lastSessionName.isEmpty() ? m_sanitizedSessionName : m_lastSessionName;
    const QString outputDir = m_lastOutputDir.isEmpty() ? m_outputDir : m_lastOutputDir;
    return QDir(outputDir).filePath(session + "_decoded");
}
QStringList PipelineController::logMessages() const { return m_logMessages; }
double PipelineController::getDecodeProgress() const { return m_decodeProgress; }

void PipelineController::setSessionName(const QString& name) {
    const QString sanitized = sanitizeSessionName(name);
    if (m_sessionName != sanitized) {
        m_sessionName = sanitized;
        emit sessionNameChanged(m_sessionName);
        emit sessionArchiveChanged();
    }
    refreshReadiness();
}

void PipelineController::setOutputDir(const QString& dir) {
    const QString cleaned = QDir::cleanPath(dir.trimmed());
    if (m_outputDir != cleaned) {
        m_outputDir = cleaned;
        emit outputDirChanged(m_outputDir);
        emit sessionArchiveChanged();
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

void PipelineController::log(const QString& message) {
    const QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    m_logMessages.append(timestamp + " " + message);
    if (m_logMessages.size() > 100) {
        m_logMessages.removeFirst();
    }
    emit logMessagesChanged();
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

void PipelineController::refreshReadiness() {
    const RecordingPreflightResult preflight = validateRecordingSetup(
        m_cameraInventoryModel->devices(),
        m_selectedCameraIndex,
        m_sessionName,
        m_outputDir,
        m_selectedResolution,
        m_requestedFps,
        m_isRecording || m_isDecoding
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

    if (m_sessionState == "idle") {
        setSessionState("idle", m_readinessMessage);
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

void PipelineController::setSessionState(const QString& state, const QString& detail) {
    bool changed = false;
    if (m_sessionState != state) {
        m_sessionState = state;
        changed = true;
    }
    if (!detail.isNull() && m_statusDetail != detail) {
        m_statusDetail = detail;
        changed = true;
    }
    if (changed) {
        emit sessionStateChanged();
    }
}

void PipelineController::setErrorState(const QString& errorMsg) {
    if (m_lastErrorMessage != errorMsg) {
        m_lastErrorMessage = errorMsg;
        emit errorStateChanged();
    }
    setDecodingState(false);
    setSessionState("error", errorMsg);
    log("Error: " + errorMsg);
    emit errorOccurred(errorMsg);
    refreshReadiness();
}

void PipelineController::setDecodingState(bool decoding) {
    if (m_isDecoding != decoding) {
        m_isDecoding = decoding;
        emit decodeStateChanged();
        refreshReadiness();
    }
}

void PipelineController::startRecording() {
    if (!m_canStartRecording) {
        setErrorState(m_readinessMessage);
        return;
    }

    const QString backend = currentBackendId();
    const int deviceIndex = currentDeviceIndex();
    const QStringList resolutionParts = m_selectedResolution.split("x");
    if (resolutionParts.size() != 2) {
        setErrorState("The selected resolution is invalid.");
        return;
    }

    const int width = resolutionParts.at(0).toInt();
    const int height = resolutionParts.at(1).toInt();
    const double fps = m_requestedFps;

    try {
        micecam::CameraConfig camConfig;
        camConfig.width = width;
        camConfig.height = height;
        camConfig.fps = fps;
        camConfig.device_id = deviceIndex;

        log("Starting capture with backend: " + backend + " (" + m_selectedResolution + " @ " + QString::number(fps) + " FPS)");
        if (!m_lastErrorMessage.isEmpty()) {
            m_lastErrorMessage.clear();
            emit errorStateChanged();
        }
        m_decodeProgress = 0.0;
        emit decodeProgressChanged(m_decodeProgress);
        m_decodedOutputDir.clear();
        emit sessionArchiveChanged();

        if (backend == "oak") {
#ifdef WITH_OAK_CAMERA
            m_cameraBackend = std::make_unique<micecam::OAKCameraBackend>();
#else
            setErrorState("OAK camera support is not available in this build.");
            return;
#endif
        } else if (backend == "ffmpeg") {
#ifdef WITH_FFMPEG
            m_cameraBackend = std::make_unique<micecam::FFmpegCameraBackend>();
#else
            setErrorState("FFmpeg camera support is not available in this build.");
            return;
#endif
        } else {
            setErrorState("Unknown camera backend selected.");
            return;
        }

        if (!m_cameraBackend->initialize(camConfig)) {
            setErrorState("Failed to initialize the selected camera.");
            return;
        }

        micecam::SessionConfig sessionConfig;
        sessionConfig.output_dir = m_outputDir.toStdString();
        sessionConfig.session_name = m_sanitizedSessionName.toStdString();
        sessionConfig.width = width;
        sessionConfig.height = height;
        sessionConfig.fps = fps;
        sessionConfig.camera_backend_name = backend.toStdString();
        sessionConfig.append = false;

        m_pipeline = std::make_unique<micecam::IngestionPipeline>(std::move(m_cameraBackend), sessionConfig);

        if (m_videoProvider) {
            m_videoProvider->setPipeline(m_pipeline.get());
            log("Preview provider attached.");
        }

        if (!m_pipeline->start()) {
            setErrorState("Failed to start the ingestion pipeline.");
            return;
        }

        m_isRecording = true;
        m_lastOutputDir = m_outputDir;
        m_currentFps = 0.0;
        m_capturedFrames = 0;
        m_droppedFrames = 0;
        m_mbps = 0.0;
        m_format = backend == "oak" ? "MJPEG" : "MJPEG";
        emit captureStarted();
        emit statsUpdated();
        emit isRecordingChanged(true);
        m_recordingTimer.start();
        setSessionState("recording", "Recording now. Watch preview and capture health.");

        m_statsTimerId = startTimer(1000);
        refreshReadiness();
    } catch (const std::exception& e) {
        setErrorState(QString::fromStdString(e.what()));
    }
}

void PipelineController::stopRecording() {
    if (!m_isRecording) {
        return;
    }

    if (m_statsTimerId > 0) {
        killTimer(m_statsTimerId);
        m_statsTimerId = 0;
    }

    if (m_pipeline) {
        if (m_videoProvider) {
            m_videoProvider->setPipeline(nullptr);
        }
        m_pipeline->stop();
        m_pipeline.reset();
        m_cameraBackend.reset();
    }

    m_isRecording = false;
    emit isRecordingChanged(false);
    m_lastSessionName = m_sanitizedSessionName;
    emit sessionArchiveChanged();
    log("Recording stopped. Session saved.");
    refreshReadiness();

    if (m_autoDecode) {
        const QString outputDir = m_outputDir;
        const QString sessionName = m_sanitizedSessionName;
        const QString targetDir = QDir(m_outputDir).filePath(m_sanitizedSessionName + "_decoded");

        setDecodingState(true);
        setSessionState("decoding", "Recording finished. Preparing export.");
        log("Auto-decode enabled. Launching native C++ decoder...");

        auto decodeTask = QtConcurrent::run([this, outputDir, sessionName, targetDir]() {
            QMetaObject::invokeMethod(this, [this]() {
                m_decodeProgress = 5.0;
                emit decodeProgressChanged(m_decodeProgress);
            }, Qt::QueuedConnection);

            auto progressCallback = [this](float progress) {
                QMetaObject::invokeMethod(this, [this, progress]() {
                    m_decodeProgress = progress;
                    emit decodeProgressChanged(m_decodeProgress);
                }, Qt::QueuedConnection);
            };

            const bool ok = m_decoder->decode_micecam_project(
                outputDir.toStdString(),
                sessionName.toStdString(),
                targetDir.toStdString(),
                progressCallback
            );

            QMetaObject::invokeMethod(this, [this, ok, targetDir]() {
                if (ok) {
                    log("Native decoding finished successfully. Exported to: " + targetDir);
                    m_decodeProgress = 100.0;
                    m_decodedOutputDir = targetDir;
                    emit sessionArchiveChanged();
                    setDecodingState(false);
                    setSessionState("completed", "Export is ready. Review the files or start another session.");
                } else {
                    m_decodeProgress = 0.0;
                    setErrorState("Native decoding failed. Check project files.");
                }
                emit decodeProgressChanged(m_decodeProgress);
            }, Qt::QueuedConnection);
        });
        Q_UNUSED(decodeTask);
    } else {
        setDecodingState(false);
        setSessionState("completed", "Raw session data is saved and ready.");
    }

    setSessionName(QStringLiteral("session_%1").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
    ));
}

void PipelineController::timerEvent(QTimerEvent* event) {
    if (event->timerId() != m_statsTimerId || !m_pipeline) {
        return;
    }

    const auto stats = m_pipeline->get_stats();
    m_currentFps = static_cast<double>(stats.captured_frames - m_capturedFrames);
    m_capturedFrames = stats.captured_frames;
    m_droppedFrames = stats.dropped_frames;
    m_mbps = stats.current_throughput_mbps;
    emit statsUpdated();
}

void PipelineController::setVideoProvider(VideoFrameProvider* provider) {
    m_videoProvider = provider;
}

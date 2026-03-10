#include "PipelineController.h"
#include "VideoFrameProvider.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QTimerEvent>
#include <QMediaDevices>
#include <QCameraDevice>
#include <thread>
#include "micecam/pipeline/ingestion_pipeline.h"
#include "micecam/camera/camera_backend.h"
#include "micecam/pipeline/decoder.h"
#include <QtConcurrent/QtConcurrent>

#ifdef WITH_OAK_CAMERA
#include "micecam/camera/oak_camera_backend.h"
#endif

#ifdef WITH_FFMPEG
#include "micecam/camera/ffmpeg_camera_backend.h"
#endif

PipelineController::PipelineController(QObject *parent)
    : QObject(parent)
{
    m_decoder = std::make_unique<micecam::Decoder>();
    // Auto-generate a fallback session name
    m_sessionName = "session_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
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
QStringList PipelineController::logMessages() const { return m_logMessages; }
double PipelineController::getDecodeProgress() const { return m_decodeProgress; }


void PipelineController::setSessionName(const QString& name) {
    if (m_sessionName != name) {
        m_sessionName = name;
        emit sessionNameChanged(m_sessionName);
    }
}

void PipelineController::setOutputDir(const QString& dir) {
    if (m_outputDir != dir) {
        m_outputDir = dir;
        emit outputDirChanged(m_outputDir);
    }
}

void PipelineController::setAutoDecode(bool enable) {
    if (m_autoDecode != enable) {
        m_autoDecode = enable;
        emit autoDecodeChanged(m_autoDecode);
    }
}

void PipelineController::log(const QString& message) {
    QString ts = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    m_logMessages.append(ts + " " + message);
    if (m_logMessages.size() > 100) m_logMessages.removeFirst();
    emit logMessagesChanged();
}


QVariantList PipelineController::getAvailableCameras() {
    QVariantList list;

#ifdef WITH_OAK_CAMERA
    QVariantMap oak;
    oak["name"] = "Luxonis OAK (DepthAI)";
    oak["id"] = "oak";
    oak["type"] = 0; // custom backend type
    list.append(oak);
#endif

#ifdef WITH_FFMPEG
    const auto cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        QVariantMap webcam;
        webcam["name"] = "System Webcam (Default)";
        webcam["id"] = "ffmpeg";
        webcam["type"] = 0;
        list.append(webcam);
    } else {
        for (int i = 0; i < cameras.size(); ++i) {
            QVariantMap webcam;
            webcam["name"] = cameras[i].description();
            webcam["id"] = "ffmpeg";
            webcam["type"] = i;
            list.append(webcam);
        }
    }
#endif

    return list;
}

QVariantList PipelineController::getAvailableResolutions(const QString& backend) {
    QVariantList list;
    if (backend == "oak") {
        list.append("1280x800");
        list.append("1280x720");
        list.append("1920x1080");
    } else {
        list.append("1920x1080");
        list.append("1280x720");
        list.append("640x480");
    }
    return list;
}

void PipelineController::startRecording(const QString& backend, int type, int width, int height, double fps) {
    if (m_isRecording) {
        qWarning() << "Pipeline is already recording!";
        return;
    }

    try {
        micecam::CameraConfig cam_config;
        cam_config.width = width;
        cam_config.height = height;
        cam_config.fps = fps;
        cam_config.device_id = type; // Generic ID handling

        log("Starting capture with backend: " + backend + " (" + QString::number(width) + "x" + QString::number(height) + ")");


        if (backend == "oak") {
#ifdef WITH_OAK_CAMERA
            m_cameraBackend = std::make_unique<micecam::OAKCameraBackend>();
#else
            emit errorOccurred("OAK camera support not compiled");
            return;
#endif
        } else {
#ifdef WITH_FFMPEG
            m_cameraBackend = std::make_unique<micecam::FFmpegCameraBackend>();
#else
            emit errorOccurred("Webcam support not compiled");
            return;
#endif
        }

        if (!m_cameraBackend->initialize(cam_config)) {
            emit errorOccurred("Failed to initialize camera backend " + backend);
            return;
        }

        micecam::SessionConfig session_config;
        session_config.output_dir = m_outputDir.toStdString();

        // Ensure output dir exists
        QDir().mkpath(m_outputDir);

        session_config.session_name = m_sessionName.toStdString();
        session_config.width = width;
        session_config.height = height;
        session_config.fps = fps;
        session_config.camera_backend_name = backend.toStdString();
        session_config.append = false;

        m_pipeline = std::make_unique<micecam::IngestionPipeline>(std::move(m_cameraBackend), session_config);

        // Zero-Drop: Attach the pipeline to the image provider for pull-based preview
        if (m_videoProvider) {
            m_videoProvider->setPipeline(m_pipeline.get());
            log("Configured VideoFrameProvider for pull-based live preview.");
        }


        if (!m_pipeline->start()) {
            emit errorOccurred("Failed to start ingestion pipeline");
            return;
        }

        m_isRecording = true;
        emit isRecordingChanged(true);
        emit captureStarted();

        // Start stats polling
        m_statsTimerId = startTimer(1000);

        qInfo() << "Recording started. Session:" << m_sessionName;

    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void PipelineController::stopRecording() {
    if (!m_isRecording) return;

    qInfo() << "Stopping recording...";
    if (m_statsTimerId > 0) {
        killTimer(m_statsTimerId);
        m_statsTimerId = 0;
    }

    if (m_pipeline) {
        if (m_videoProvider) m_videoProvider->setPipeline(nullptr);
        m_pipeline->stop();
        m_pipeline.reset();
        m_cameraBackend.reset(); // Safely shut down device
    }

    m_isRecording = false;
    emit isRecordingChanged(false);
    log("Recording stopped. Session saved.");

    if (m_autoDecode) {
        log("Auto-decode enabled. Launching native C++ decoder...");

        std::string outputDir = m_outputDir.toStdString();
        std::string sessionName = m_sessionName.toStdString();
        std::string targetParentDir = (QDir(m_outputDir).filePath(m_sessionName + "_decoded")).toStdString();

        QtConcurrent::run([this, outputDir, sessionName, targetParentDir]() {
            m_decodeProgress = 5.0;
            emit decodeProgressChanged(m_decodeProgress);

            auto cb = [this](float p) {
                m_decodeProgress = p;
                emit decodeProgressChanged(m_decodeProgress);
            };

            if (m_decoder->decode_micecam_project(outputDir, sessionName, targetParentDir, cb)) {
                log("Native decoding finished successfully. Exported to: " + QString::fromStdString(targetParentDir));
                m_decodeProgress = 100.0;
            } else {
                log("Native decoding failed. Check project files.");
                m_decodeProgress = 0.0;
            }
            emit decodeProgressChanged(m_decodeProgress);
        });
    }

    // Refresh session name immediately for next recording
    setSessionName("session_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
}

void PipelineController::timerEvent(QTimerEvent *event) {
    if (event->timerId() == m_statsTimerId && m_pipeline) {
        auto stats = m_pipeline->get_stats();
        m_currentFps = static_cast<double>(stats.captured_frames - m_capturedFrames); // 1s interval
        m_capturedFrames = stats.captured_frames;
        m_droppedFrames = stats.dropped_frames;
        m_mbps = stats.current_throughput_mbps;
        emit statsUpdated();
    }
}

void PipelineController::setVideoProvider(VideoFrameProvider* provider) {
    m_videoProvider = provider;
}

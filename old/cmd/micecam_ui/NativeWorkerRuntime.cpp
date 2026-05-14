#include "NativeWorkerRuntime.h"

#include <QCoreApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QSocketNotifier>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>

#include <cstring>
#include <cstdio>
#include <opencv2/opencv.hpp>

#include "micecam/camera/camera_backend.h"
#include "micecam/pipeline/decoder.h"
#include "micecam/pipeline/ingestion_pipeline.h"

#ifdef WITH_OAK_CAMERA
#include "micecam/camera/oak_camera_backend.h"
#endif

#ifdef WITH_FFMPEG
#include "micecam/camera/ffmpeg_camera_backend.h"
#endif

namespace micecam_ui {

namespace {

void writeJsonLine(const QJsonObject& object) {
    QTextStream stream(stdout);
    stream << QJsonDocument(object).toJson(QJsonDocument::Compact) << Qt::endl;
}

}  // namespace

NativeWorkerRuntime::NativeWorkerRuntime(QObject* parent) : QObject(parent) {
    m_decoder = std::make_unique<micecam::Decoder>();
}

NativeWorkerRuntime::~NativeWorkerRuntime() {
    if (m_statsTimerId > 0) {
        killTimer(m_statsTimerId);
    }
    if (m_previewTimerId > 0) {
        killTimer(m_previewTimerId);
    }
}

void NativeWorkerRuntime::start() {
    m_stdinFile = new QFile(this);
    const bool opened = m_stdinFile->open(
        stdin,
        QIODevice::ReadOnly | QIODevice::Text,
        QFileDevice::DontCloseHandle
    );
    Q_UNUSED(opened);

    m_stdinNotifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this);
    connect(m_stdinNotifier, &QSocketNotifier::activated, this, [this]() { processIncomingData(); });

    writeJsonLine({
        {"type", "hello"},
        {"detail", "Recording worker ready."},
        {"previewAvailable", false},
        {"previewMode", "offline"},
        {"previewDetail", "Preview is offline until recording starts."},
    });
}

void NativeWorkerRuntime::processIncomingData() {
    if (!m_stdinFile) {
        return;
    }

    m_inputBuffer.append(m_stdinFile->readAll());
    while (true) {
        const int newlineIndex = m_inputBuffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        const QByteArray line = m_inputBuffer.left(newlineIndex).trimmed();
        m_inputBuffer.remove(0, newlineIndex + 1);
        if (!line.isEmpty()) {
            processCommand(line);
        }
    }
}

void NativeWorkerRuntime::processCommand(const QByteArray& line) {
    const QJsonDocument document = QJsonDocument::fromJson(line);
    if (!document.isObject()) {
        publishActivity("warning", "system", "Worker received invalid command payload.");
        return;
    }

    const QJsonObject object = document.object();
    const QString type = object.value("type").toString();
    if (type == "start") {
        startRecording(object);
    } else if (type == "stop") {
        stopRecording();
    } else if (type == "shutdown") {
        requestShutdown();
    }
}

void NativeWorkerRuntime::startRecording(const QJsonObject& command) {
    if (m_isRecording || m_isDecoding) {
        publishStatus("error", "Worker is busy.");
        return;
    }

    m_backendId = command.value("backendId").toString();
    m_outputDir = command.value("outputDir").toString();
    m_sessionName = command.value("sessionName").toString();
    m_autoDecode = command.value("autoDecode").toBool(true);
    m_previewEnabled = command.value("previewEnabled").toBool(true);
    m_exportPath = QDir(m_outputDir).filePath(m_sessionName + "_decoded");

    micecam::CameraConfig camConfig;
    camConfig.width = command.value("width").toInt();
    camConfig.height = command.value("height").toInt();
    camConfig.fps = command.value("fps").toDouble();
    camConfig.device_id = command.value("deviceIndex").toInt();
    camConfig.device_name = command.value("deviceName").toString().toStdString();

    try {
        if (m_backendId == "oak") {
#ifdef WITH_OAK_CAMERA
            m_cameraBackend = std::make_unique<micecam::OAKCameraBackend>();
#else
            publishStatus("error", "OAK camera support is not available in this build.");
            return;
#endif
        } else if (m_backendId == "ffmpeg") {
#ifdef WITH_FFMPEG
            m_cameraBackend = std::make_unique<micecam::FFmpegCameraBackend>();
#else
            publishStatus("error", "FFmpeg camera support is not available in this build.");
            return;
#endif
        } else {
            publishStatus("error", "Unknown camera backend selected.");
            return;
        }

        if (!m_cameraBackend->initialize(camConfig)) {
            publishStatus("error", "Failed to initialize the selected camera.");
            return;
        }

        micecam::SessionConfig sessionConfig;
        sessionConfig.output_dir = m_outputDir.toStdString();
        sessionConfig.session_name = m_sessionName.toStdString();
        sessionConfig.width = camConfig.width;
        sessionConfig.height = camConfig.height;
        sessionConfig.fps = camConfig.fps;
        sessionConfig.camera_backend_name = m_backendId.toStdString();
        sessionConfig.append = false;

        m_pipeline = std::make_unique<micecam::IngestionPipeline>(std::move(m_cameraBackend), sessionConfig);
        if (!m_pipeline->start()) {
            publishStatus("error", "Failed to start the ingestion pipeline.");
            return;
        }

        m_isRecording = true;
        m_lastCapturedFrames = 0;
        m_lastCurrentFps = 0.0;
        if (m_statsTimerId > 0) {
            killTimer(m_statsTimerId);
        }
        if (m_previewTimerId > 0) {
            killTimer(m_previewTimerId);
        }
        m_statsTimerId = startTimer(1000);
        m_previewTimerId = m_previewEnabled ? startTimer(200) : 0;
        publishActivity("info", "session", "Recording started.", QDir(m_outputDir).filePath(m_sessionName));
        publishStatus("recording", "Recording now. Watch preview and capture health.");
    } catch (const std::exception& exception) {
        publishStatus("error", QString::fromStdString(exception.what()));
    }
}

void NativeWorkerRuntime::stopRecording() {
    if (!m_isRecording) {
        return;
    }

    publishStatus("stopping", "Stopping recording safely.");

    if (m_statsTimerId > 0) {
        killTimer(m_statsTimerId);
        m_statsTimerId = 0;
    }
    if (m_previewTimerId > 0) {
        killTimer(m_previewTimerId);
        m_previewTimerId = 0;
    }
    if (m_pipeline) {
        m_pipeline->stop();
        m_pipeline.reset();
    }

    m_isRecording = false;
    publishActivity("info", "session", "Recording stopped.", QDir(m_outputDir).filePath(m_sessionName));

    if (!m_autoDecode) {
        finishCompletedState();
        return;
    }

    m_isDecoding = true;
    publishStatus("decoding", "Recording finished. Preparing export.", 0.0);

    const QString outputDir = m_outputDir;
    const QString sessionName = m_sessionName;
    const QString exportPath = m_exportPath;
    auto decodeTask = QtConcurrent::run([this, outputDir, sessionName, exportPath]() {
        auto progressCallback = [this](float progress) {
            QMetaObject::invokeMethod(this, [this, progress]() {
                publishStatus("decoding", "Recording finished. Preparing export.", progress);
            }, Qt::QueuedConnection);
        };

        const bool ok = m_decoder->decode_micecam_project(
            outputDir.toStdString(),
            sessionName.toStdString(),
            exportPath.toStdString(),
            progressCallback
        );

        QMetaObject::invokeMethod(this, [this, ok]() {
            m_isDecoding = false;
            if (!ok) {
                publishStatus("error", "Native decoding failed. Check project files.");
                if (m_shutdownRequested) {
                    QCoreApplication::quit();
                }
                return;
            }

            publishActivity("info", "export", "Decoded export is ready.", m_exportPath);
            finishCompletedState();
        }, Qt::QueuedConnection);
    });
    Q_UNUSED(decodeTask);
}

void NativeWorkerRuntime::requestShutdown() {
    m_shutdownRequested = true;
    if (m_isRecording) {
        stopRecording();
        return;
    }
    if (m_isDecoding) {
        publishActivity("warning", "system", "Shutdown requested while decode is still running.");
        return;
    }
    QCoreApplication::quit();
}

void NativeWorkerRuntime::publishStatus(const QString& state, const QString& detail, double decodeProgress) {
    QJsonObject object{
        {"type", "status"},
        {"state", state},
        {"detail", detail},
        {"resolvedSessionPath", QDir(m_outputDir).filePath(m_sessionName)},
        {"resolvedExportPath", m_exportPath},
        {"previewAvailable", m_isRecording && m_previewEnabled},
        {"previewMode", m_isRecording ? (m_previewEnabled ? "capped" : "disabled") : "offline"},
        {"previewDetail", m_isRecording
            ? (m_previewEnabled
                ? "Preview is capped at 5 FPS with latest-frame-only delivery."
                : "Preview is disabled for recording stability.")
            : "Preview is offline until recording starts."},
    };

    if (decodeProgress >= 0.0) {
        object.insert("decodeProgress", decodeProgress);
    }

    if (m_pipeline) {
        const auto stats = m_pipeline->get_stats();
        object.insert("capturedFrames", static_cast<qint64>(stats.captured_frames));
        object.insert("droppedFrames", static_cast<qint64>(stats.dropped_frames));
        object.insert("throughputMbps", stats.current_throughput_mbps);
        object.insert("currentFps", m_lastCurrentFps);
    }

    writeJsonLine(object);
}

void NativeWorkerRuntime::publishActivity(
    const QString& severity,
    const QString& category,
    const QString& message,
    const QString& relatedPath
) {
    writeJsonLine({
        {"type", "activity"},
        {"severity", severity},
        {"category", category},
        {"message", message},
        {"relatedPath", relatedPath},
    });
}

void NativeWorkerRuntime::finishCompletedState() {
    publishStatus("completed", m_autoDecode
        ? "Export is ready. Review the files or start another session."
        : "Raw session data is saved and ready.");
    if (m_shutdownRequested) {
        QCoreApplication::quit();
    }
}

void NativeWorkerRuntime::timerEvent(QTimerEvent* event) {
    if (event->timerId() == m_statsTimerId) {
        if (!m_pipeline || !m_isRecording) {
            return;
        }

        const auto stats = m_pipeline->get_stats();
        m_lastCurrentFps = static_cast<double>(stats.captured_frames - m_lastCapturedFrames);
        m_lastCapturedFrames = stats.captured_frames;
        publishStatus("recording", "Recording now. Watch preview and capture health.");
        return;
    }

    if (event->timerId() == m_previewTimerId) {
        publishPreviewFrame();
    }
}

void NativeWorkerRuntime::publishPreviewFrame() {
    if (!m_pipeline || !m_isRecording || !m_previewEnabled) {
        return;
    }

    std::unique_ptr<micecam::Frame> frame = m_pipeline->get_preview_frame();
    if (!frame) {
        return;
    }

    QImage image;
    if (frame->format == micecam::PixelFormat::MONO8) {
        image = QImage(frame->width, frame->height, QImage::Format_Grayscale8);
        std::memcpy(image.bits(), frame->data->data(), frame->data->size());
    } else if (frame->format == micecam::PixelFormat::UYVY422) {
        cv::Mat yuv(frame->height, frame->width, CV_8UC2, static_cast<void*>(frame->data->data()));
        cv::Mat rgb;
        cv::cvtColor(yuv, rgb, cv::COLOR_YUV2RGB_UYVY);
        image = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    } else if (frame->format == micecam::PixelFormat::RGB24) {
        image = QImage(frame->width, frame->height, QImage::Format_RGB888);
        std::memcpy(image.bits(), frame->data->data(), frame->data->size());
    } else {
        image.loadFromData(frame->data->data(), frame->data->size(), "JPEG");
    }

    if (image.isNull()) {
        return;
    }

    const QImage scaled = image.scaled(640, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QByteArray jpegBytes;
    QBuffer buffer(&jpegBytes);
    if (!buffer.open(QIODevice::WriteOnly) || !scaled.save(&buffer, "JPEG", 60)) {
        return;
    }

    writeJsonLine({
        {"type", "preview"},
        {"jpegBase64", QString::fromLatin1(jpegBytes.toBase64())},
    });
}

}  // namespace micecam_ui

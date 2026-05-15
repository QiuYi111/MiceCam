#include "AppController.h"

#include <chrono>
#include <sstream>

#include "domain/Capabilities.h"
#include "pipeline/PreflightValidator.h"

namespace micecam::ui {

AppController::AppController(BackendMode mode, QObject* parent)
    : QObject(parent)
    , mode_(mode)
    , camera_model_(new AppCameraModel(this))
    , alert_model_(new AppAlertModel(this))
    , settings_(new AppSettings(this))
{
    if (mode_ == BackendMode::MockOnly) {
        manager_.register_backend(std::make_unique<infrastructure::MockCameraBackend>());
    }
}

QAbstractListModel* AppController::cameraModel() const { return camera_model_; }
QAbstractListModel* AppController::alertModel() const { return alert_model_; }
AppSettings* AppController::settings() const { return settings_; }

bool AppController::isRecording() const { return recording_; }

QString AppController::recordButtonText() const {
    return recording_ ? QStringLiteral("Stop") : QStringLiteral("Record");
}

QString AppController::cameraCountText() const {
    int count = camera_model_->rowCount();
    return QStringLiteral("%1 cameras").arg(count);
}

QString AppController::elapsedText() const {
    return QStringLiteral("00:00");
}

QString AppController::totalFramesText() const {
    return QString::number(total_frames_);
}

QString AppController::averageFpsText() const {
    return QString::number(average_fps_, 'f', 1);
}

QString AppController::bytesWrittenText() const {
    return QString::number(bytes_written_ / (1024 * 1024)) + QStringLiteral(" MB");
}

QString AppController::diskRemainingText() const {
    return disk_remaining_.isEmpty() ? QStringLiteral("N/A") : disk_remaining_;
}

QString AppController::preflightMessage() const {
    return preflight_message_.isEmpty() ? QStringLiteral("Ready") : preflight_message_;
}

QString AppController::lastSessionId() const {
    return session_id_;
}

void AppController::setOutputDirectory(const QString& dir) {
    output_dir_ = dir;
}

void AppController::refreshCameras() {
    auto devices = manager_.discover_all();

    std::vector<CameraRow> rows;
    for (const auto& device : devices) {
        for (const auto& stream : device.streams) {
            CameraRow row;
            row.cameraId = QString::fromStdString(device.id) + "_" + QString::number(stream.index);
            row.name = QString::fromStdString(stream.label);
            row.fps = stream.supported_framerates.empty() ? 0.0 : static_cast<double>(stream.supported_framerates.front());
            row.dropCount = 0;
            row.recording = recording_;
            row.status = 0;
            row.alertMessage.clear();

            for (const auto& res : stream.resolutions) {
                row.resolutionLabels.append(
                    QString::asprintf("%d x %d", res.width, res.height));
            }
            for (int fr : stream.supported_framerates) {
                row.framerateLabels.append(QStringLiteral("%1 fps").arg(fr));
            }
            for (const auto& fmt : stream.supported_formats) {
                row.formatLabels.append(QString::fromStdString(fmt));
            }
            rows.push_back(std::move(row));
        }
    }

    camera_model_->replaceRows(std::move(rows));
    emit cameraCountTextChanged();
}

bool AppController::startRecording() {
    if (recording_) return false;

    if (camera_model_->rowCount() == 0) return false;

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::ostringstream sid;
    sid << "session_" << now;
    session_id_ = QString::fromStdString(sid.str());

    pipeline::SessionConfig config;
    config.session_id = session_id_.toStdString();
    config.output_dir = output_dir_.isEmpty()
        ? "."
        : output_dir_.toStdString();

    auto devices = manager_.discover_all();
    for (const auto& device : devices) {
        for (const auto& stream : device.streams) {
            domain::StreamConfig sc;
            sc.device_id = device.id;
            sc.stream_index = stream.index;
            sc.width = stream.max_width;
            sc.height = stream.max_height;
            sc.framerate = stream.supported_framerates.empty() ? 30 : stream.supported_framerates.front();
            sc.pixel_format = stream.supported_formats.empty() ? "rgb24" : stream.supported_formats.front();
            config.streams.push_back(std::move(sc));
        }
    }

    if (!pipeline_.start(config)) return false;

    recording_ = true;
    emit isRecordingChanged();
    emit recordButtonTextChanged();
    emit lastSessionIdChanged();
    return true;
}

void AppController::stopRecording() {
    if (!recording_) return;

    pipeline_.stop();

    auto [meta, stats_vec] = pipeline_.result();
    total_frames_ = 0;
    bytes_written_ = 0;
    double total_fps = 0.0;
    int stats_count = 0;

    for (const auto& s : stats_vec) {
        total_frames_ += s.frames_actual;
        bytes_written_ += s.bytes_written;
        if (s.frames_actual > 0) {
            total_fps += s.frames_actual;
            stats_count++;
        }
    }
    if (stats_count > 0) {
        average_fps_ = total_fps / stats_count;
    }

    recording_ = false;
    emit isRecordingChanged();
    emit recordButtonTextChanged();
    emit totalFramesTextChanged();
    emit averageFpsTextChanged();
    emit bytesWrittenTextChanged();
}

QVariantList AppController::preflightItems() {
    QVariantList items;
    return items;
}

QVariantMap AppController::cameraAt(int row) {
    return camera_model_->get(row);
}

} // namespace micecam::ui

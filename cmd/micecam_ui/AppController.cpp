#include "AppController.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "domain/Capabilities.h"
#include "domain/PluginManifest.h"
#include "pipeline/PreflightValidator.h"

namespace micecam::ui {

namespace fs = std::filesystem;

AppController::AppController(QObject* parent)
    : QObject(parent)
    , plugin_registry_("../3rdParty/bundled_plugins", ".")
    , camera_model_(new AppCameraModel(this))
    , source_model_(new CameraSourceModel(this))
    , alert_model_(new AppAlertModel(this))
    , settings_(new AppSettings(this))
{
    plugin_registry_.initialize();
    manager_.set_plugin_registry(&plugin_registry_);
    setupCrashAlertHandler();
}

QAbstractListModel* AppController::cameraModel() const { return camera_model_; }
QAbstractListModel* AppController::sourceModel() const { return source_model_; }
QAbstractListModel* AppController::alertModel() const { return alert_model_; }
AppSettings* AppController::settings() const { return settings_; }

bool AppController::isRecording() const { return recording_; }

QString AppController::recordButtonText() const {
    if (recording_) {
        return QStringLiteral("Stop");
    }
    return canStartRecording() ? QStringLiteral("Start") : QStringLiteral("No Device");
}

bool AppController::canStartRecording() const {
    return !recording_ && camera_model_->rowCount() > 0;
}

int AppController::cameraCount() const {
    return camera_model_->rowCount();
}

QString AppController::cameraCountText() const {
    return QStringLiteral("%1 cameras").arg(cameraCount());
}

QString AppController::elapsedText() const {
    if (session_start_ == std::chrono::steady_clock::time_point{}) {
        return QStringLiteral("00:00");
    }
    auto elapsed = std::chrono::steady_clock::now() - session_start_;
    auto total_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    int min = static_cast<int>(total_sec / 60);
    int sec = static_cast<int>(total_sec % 60);
    return QString::asprintf("%02d:%02d", min, sec);
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
    if (!preflight_message_.isEmpty()) {
        return preflight_message_;
    }
    return cameraCount() == 0 ? QStringLiteral("No cameras detected") : QStringLiteral("Ready");
}

QString AppController::lastSessionId() const {
    return session_id_;
}

QStringList AppController::recentLogEntries() const {
    return log_entries_;
}

void AppController::setOutputDirectory(const QString& dir) {
    output_dir_ = dir;
}

static void pushLogEntry(QStringList& entries, const QString& msg) {
    entries.append(msg);
    if (entries.size() > 50) {
        entries.removeFirst();
    }
}

void AppController::refreshCameras() {
    auto devices = manager_.discover_all();
    auto sources = manager_.get_sources();

    std::vector<CameraRow> rows;
    for (const auto& device : devices) {
        for (const auto& stream : device.streams) {
            CameraRow row;
            row.cameraId = QString::fromStdString(device.id) + "_" + QString::number(stream.index);
            row.name = QString::fromStdString(stream.label);
            row.sourceId.clear();
            row.sourceGroup.clear();
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

    const int previous_count = cameraCount();
    camera_model_->replaceRows(std::move(rows));

    std::vector<domain::PluginDeviceInfo> plugin_devices;
    source_model_->populateFromSources(sources, plugin_devices);

    preflight_message_ = cameraCount() == 0
        ? QStringLiteral("No cameras detected")
        : QStringLiteral("Ready");
    pushLogEntry(log_entries_, QStringLiteral("[INFO] Camera refresh: %1").arg(cameraCountText()));
    if (cameraCount() != previous_count) {
        emit cameraCountChanged();
        emit canStartRecordingChanged();
        emit recordButtonTextChanged();
    }
    emit cameraCountTextChanged();
    emit preflightMessageChanged();
}

bool AppController::startRecording() {
    if (recording_) return false;

    if (camera_model_->rowCount() == 0) {
        preflight_message_ = QStringLiteral("No cameras detected");
        emit preflightMessageChanged();
        emit canStartRecordingChanged();
        emit recordButtonTextChanged();
        return false;
    }

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

    pushLogEntry(log_entries_, QStringLiteral("[INFO] Session started: %1").arg(session_id_));
    pushLogEntry(log_entries_, QStringLiteral("[INFO] %1").arg(cameraCountText()));

    active_streams_.clear();
    for (const auto& stream_config : config.streams) {
        auto stream = manager_.open_stream(stream_config);
        if (stream) {
            active_streams_.push_back({stream_config, std::move(stream)});
        }
    }

    capture_running_ = true;
    capture_thread_ = std::thread([this] { captureLoop(); });

    session_start_ = std::chrono::steady_clock::now();
    recording_ = true;
    emit isRecordingChanged();
    emit canStartRecordingChanged();
    emit recordButtonTextChanged();
    emit lastSessionIdChanged();
    return true;
}

void AppController::stopRecording() {
    if (!recording_) return;

    stopCaptureLoop();
    pipeline_.stop();

    pushLogEntry(log_entries_, QStringLiteral("[INFO] Recording stopped: %1").arg(session_id_));

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
    session_start_ = {};
    emit isRecordingChanged();
    emit canStartRecordingChanged();
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

QVariantList AppController::pluginList() {
    QVariantList list;
    auto sources = plugin_registry_.getSources();
    for (const auto& src : sources) {
        QVariantMap item;
        item["pluginId"] = QString::fromStdString(src.source_id);
        item["name"] = QString::fromStdString(src.source_name);
        item["version"] = QString::fromStdString(src.plugin_version);
        item["path"] = QString::fromStdString(src.plugin_path);
        item["enabled"] = src.enabled;
        item["type"] = src.source_type == domain::PluginSourceType::BUNDLED
                            ? QStringLiteral("bundled")
                            : QStringLiteral("linked");
        item["deviceCount"] = static_cast<int>(src.device_ids.size());

        switch (src.diagnostics_state) {
            case domain::PluginDiagnosticsState::OK:
                item["status"] = QStringLiteral("OK"); break;
            case domain::PluginDiagnosticsState::ERROR:
                item["status"] = QStringLiteral("Error"); break;
            case domain::PluginDiagnosticsState::DISABLED:
                item["status"] = QStringLiteral("Disabled"); break;
            default:
                item["status"] = QStringLiteral("Missing"); break;
        }
        list.append(item);
    }
    return list;
}

bool AppController::importPlugin(const QString& dirPath) {
    if (recording_) return false;

    bool ok = plugin_registry_.addLinkedDirectory(dirPath.toStdString());
    if (ok) {
        pushLogEntry(log_entries_,
            QStringLiteral("[INFO] Plugin imported: %1 (restart required)")
                .arg(dirPath));
    } else {
        pushLogEntry(log_entries_,
            QStringLiteral("[WARN] Plugin import failed: %1").arg(dirPath));
    }
    emit pluginsChanged();
    return ok;
}

void AppController::togglePlugin(const QString& pluginPath, bool enabled) {
    if (recording_) return;

    auto sources = plugin_registry_.getSources();
    for (const auto& src : sources) {
        if (src.plugin_path == pluginPath.toStdString()) {
            if (enabled) {
                plugin_registry_.enablePlugin(src.source_id);
            } else {
                plugin_registry_.disablePlugin(src.source_id);
            }
            pushLogEntry(log_entries_,
                QStringLiteral("[INFO] Plugin %1 %2 (restart required)")
                    .arg(QString::fromStdString(src.source_id),
                         enabled ? QStringLiteral("enabled")
                                 : QStringLiteral("disabled")));
            emit pluginsChanged();
            return;
        }
    }
}

QVariantMap AppController::getPluginDetail(const QString& pluginPath) {
    QVariantMap result;

    auto manifest_path = pluginPath.toStdString() + "/plugin.json";
    if (!fs::exists(manifest_path)) {
        result["error"] = QStringLiteral("Manifest not found");
        return result;
    }

    std::ifstream file(manifest_path);
    if (!file.is_open()) {
        result["error"] = QStringLiteral("Cannot open manifest");
        return result;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error&) {
        result["error"] = QStringLiteral("Invalid JSON in manifest");
        return result;
    }

    domain::PluginManifest manifest;
    try {
        manifest = domain::PluginManifest::from_json(j);
    } catch (const std::exception&) {
        result["error"] = QStringLiteral("Failed to parse manifest");
        return result;
    }

    result["pluginId"] = QString::fromStdString(manifest.id);
    result["name"] = QString::fromStdString(manifest.name);
    result["pluginVersion"] = QString::fromStdString(manifest.version);
    result["apiVersion"] = manifest.plugin_api_version;
    result["description"] = QString::fromStdString(manifest.description);
    result["author"] = QString::fromStdString(manifest.author);

    QStringList features;
    for (const auto& f : manifest.required_features) {
        features.append(QString::fromStdString(f));
    }
    result["requiredFeatures"] = features;

    QStringList optFeatures;
    for (const auto& f : manifest.optional_features) {
        optFeatures.append(QString::fromStdString(f));
    }
    result["optionalFeatures"] = optFeatures;

    result["processModel"] = QString::fromStdString(manifest.preferred_process_model);

    QVariantMap platforms;
    for (const auto& [os, entry] : manifest.platforms) {
        platforms[QString::fromStdString(os)] =
            QString::fromStdString(entry.entrypoint);
    }
    result["platforms"] = platforms;
    result["path"] = pluginPath;

    auto sources = plugin_registry_.getSources();
    for (const auto& src : sources) {
        if (src.plugin_path == pluginPath.toStdString()) {
            result["enabled"] = src.enabled;
            result["type"] = src.source_type == domain::PluginSourceType::BUNDLED
                                ? QStringLiteral("bundled")
                                : QStringLiteral("linked");

            switch (src.diagnostics_state) {
                case domain::PluginDiagnosticsState::OK:
                    result["status"] = QStringLiteral("OK"); break;
                case domain::PluginDiagnosticsState::ERROR:
                    result["status"] = QStringLiteral("Error"); break;
                case domain::PluginDiagnosticsState::DISABLED:
                    result["status"] = QStringLiteral("Disabled"); break;
                default:
                    result["status"] = QStringLiteral("Missing"); break;
            }
            break;
        }
    }

    auto diags = plugin_registry_.getDiagnostics();
    QStringList diagList;
    for (const auto& d : diags) {
        if (d.plugin_id == manifest.id) {
            diagList.append(QStringLiteral("[%1] %2: %3")
                .arg(QString::fromStdString(d.plugin_id),
                     QString::fromStdString(d.error_code),
                     QString::fromStdString(d.message)));
        }
    }
    result["diagnostics"] = diagList;

    return result;
}

void AppController::captureLoop() {
    using namespace std::chrono;
    while (capture_running_) {
        for (auto& active : active_streams_) {
            if (!active.stream || !active.stream->is_open())
                continue;

            std::vector<uint8_t> bytes;
            int64_t pts = 0;
            if (!active.stream->read_frame(bytes, pts))
                continue;

            pipeline::FrameData frame;
            frame.stream_id = active.config.device_id + "_" +
                std::to_string(active.config.stream_index);
            frame.data = bytes.data();
            frame.size = bytes.size();
            frame.width = active.stream->width();
            frame.height = active.stream->height();
            frame.pts = pts;
            frame.source_format = active.stream->pixel_format();

            if (pipeline_.push_frame(frame)) {
                total_frames_++;
                bytes_written_ += bytes.size();
            }
        }
        QMetaObject::invokeMethod(this, [this] { refreshLiveStatus(); },
                                 Qt::QueuedConnection);
        std::this_thread::sleep_for(milliseconds(33));
    }
}

void AppController::stopCaptureLoop() {
    capture_running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    active_streams_.clear();
}

void AppController::refreshLiveStatus() {
    pushLogEntry(log_entries_, QStringLiteral("[DEBUG] Frames: %1, Elapsed: %2")
        .arg(QString::number(total_frames_), elapsedText()));
    emit elapsedTextChanged();
    emit totalFramesTextChanged();
    emit bytesWrittenTextChanged();
    emit logEntriesChanged();
}

void AppController::setupCrashAlertHandler() {
    plugin_registry_.set_crash_alert_callback([this](const std::string& plugin_id) {
        QMetaObject::invokeMethod(this, [this, plugin_id] {
            handlePluginCrash(plugin_id);
        }, Qt::QueuedConnection);
    });
}

void AppController::handlePluginCrash(const std::string& pluginId) {
    pushLogEntry(log_entries_,
        QStringLiteral("[WARN] Plugin crashed: %1").arg(QString::fromStdString(pluginId)));
    emit pluginCrashAlert(QString::fromStdString(pluginId));

    auto streams = plugin_registry_.get_streams_for_plugin(pluginId);
    for (const auto& stream_id : streams) {
        pipeline_.finalize_stream(stream_id);
    }

    auto result = plugin_registry_.handle_plugin_crash(pluginId);
    if (result.restart_succeeded) {
        pushLogEntry(log_entries_,
            QStringLiteral("[INFO] Plugin %1 restarted, starting reconnect recording")
                .arg(QString::fromStdString(pluginId)));
        int reconnect_idx = 1;
        for (const auto& stream_id : result.finalized_streams) {
            pipeline_.start_reconnect(stream_id, reconnect_idx);
        }
    }
}

void AppController::handleDeviceDisconnect(const std::string& streamId,
                                            const std::string& deviceName) {
    pushLogEntry(log_entries_,
        QStringLiteral("[WARN] Device disconnected: %1 (stream: %2)")
            .arg(QString::fromStdString(deviceName),
                 QString::fromStdString(streamId)));
    emit deviceDisconnected(QString::fromStdString(deviceName));

    if (recording_) {
        pipeline_.finalize_stream(streamId);

        for (auto& active : active_streams_) {
            std::string sid = active.config.device_id + "_" +
                std::to_string(active.config.stream_index);
            if (sid == streamId) {
                active.stream.reset();
                break;
            }
        }
    }
}

} // namespace micecam::ui

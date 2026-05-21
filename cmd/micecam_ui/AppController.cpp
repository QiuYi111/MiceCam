#include "AppController.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <QCoreApplication>

#include "domain/Capabilities.h"
#include "domain/PluginManifest.h"
#include "pipeline/PreflightValidator.h"
#include "infrastructure/FFmpegCameraBackend.h"
#include "CameraPermissionHelper.h"

namespace micecam::ui {

namespace fs = std::filesystem;

namespace {

std::string resolveBundledPluginsDir() {
    if (const char* override_dir = std::getenv("MICECAM_BUNDLED_PLUGINS_DIR")) {
        if (*override_dir != '\0') {
            return fs::absolute(override_dir).lexically_normal().string();
        }
    }

    std::vector<fs::path> candidates;
    candidates.push_back(fs::current_path() / "3rdParty" / "bundled_plugins");
    candidates.push_back(fs::current_path() / ".." / "3rdParty" / "bundled_plugins");

    const auto app_dir = fs::path(QCoreApplication::applicationDirPath().toStdString());
    candidates.push_back(app_dir / ".." / ".." / ".." / "3rdParty" / "bundled_plugins");
    candidates.push_back(app_dir / ".." / "Resources" / "3rdParty" / "bundled_plugins");

    for (const auto& candidate : candidates) {
        std::error_code ec;
        const auto normalized = fs::weakly_canonical(candidate, ec);
        const auto& path = ec ? candidate.lexically_normal() : normalized;
        if (fs::exists(path) && fs::is_directory(path)) {
            return path.string();
        }
    }

    return candidates.front().lexically_normal().string();
}

} // namespace

AppController::AppController(QObject* parent)
    : QObject(parent)
    , plugin_registry_(resolveBundledPluginsDir(), ".")
    , camera_model_(new AppCameraModel(this))
    , source_model_(new CameraSourceModel(this))
    , alert_model_(new AppAlertModel(this))
    , settings_(new AppSettings(this))
{
    plugin_registry_.initialize();
    manager_.set_plugin_registry(&plugin_registry_);
    manager_.register_backend(std::make_unique<infrastructure::FFmpegCameraBackend>());

#ifdef MICECAM_VERSION
    app_version_ = QStringLiteral(MICECAM_VERSION);
#else
    app_version_ = QStringLiteral("0.0.0");
#endif
    build_date_ = QStringLiteral(__DATE__);

    setupCrashAlertHandler();

    metrics_timer_ = new QTimer(this);
    metrics_timer_->setInterval(1000);
    connect(metrics_timer_, &QTimer::timeout, this, [this] { pushLiveMetrics(); });
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
    auto total_sec = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count());
    int hr = total_sec / 3600;
    int min = (total_sec % 3600) / 60;
    int sec = total_sec % 60;
    if (hr > 0) return QString::asprintf("%02d:%02d:%02d", hr, min, sec);
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

QString AppController::currentEncoderName() const {
    return current_encoder_name_;
}

QString AppController::currentBitrate() const {
    return current_bitrate_;
}

QStringList AppController::recentLogEntries() const {
    return log_entries_;
}

QString AppController::appVersion() const { return app_version_; }
QString AppController::buildDate() const { return build_date_; }

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
    micecam_request_camera_access();
    auto devices = manager_.discover_all();
    auto sources = manager_.get_sources();

    std::unordered_map<std::string, const domain::PluginSource*> source_by_device;
    for (const auto& source : sources) {
        for (const auto& device_id : source.device_ids) {
            source_by_device[device_id] = &source;
        }
    }
    const domain::PluginSource* fallback_source = sources.size() == 1 ? &sources.front() : nullptr;

    std::vector<CameraRow> rows;
    std::vector<domain::PluginDeviceInfo> plugin_devices;
    for (const auto& device : devices) {
        const auto source_it = source_by_device.find(device.id);
        const domain::PluginSource* source = source_it != source_by_device.end()
            ? source_it->second
            : fallback_source;

        domain::PluginDeviceInfo plugin_device;
        plugin_device.device_id = device.id;
        plugin_device.display_name = device.name.empty() ? device.id : device.name;
        plugin_device.plugin_id = source ? source->source_id : device.type;
        plugin_device.status = "available";
        plugin_device.max_width = 0;
        plugin_device.max_height = 0;
        plugin_device.max_framerate = 0.0;

        for (const auto& stream : device.streams) {
            CameraRow row;
            row.cameraId = QString::fromStdString(device.id) + "_" + QString::number(stream.index);
            row.name = QString::fromStdString(stream.label);
            row.sourceId = source ? QString::fromStdString(source->source_id) : QString::fromStdString(device.type);
            row.sourceGroup = source ? QString::fromStdString(source->source_name) : QString::fromStdString(device.type);
            row.fps = stream.supported_framerates.empty() ? 0.0 : static_cast<double>(stream.supported_framerates.front());
            row.dropCount = 0;
            row.recording = recording_;
            row.status = stream.available ? 0 : 2;
            row.alertMessage = QString::fromStdString(stream.unavailable_reason);

            for (const auto& res : stream.resolutions) {
                row.resolutionLabels.append(
                    QString::asprintf("%d x %d", res.width, res.height));
                plugin_device.max_width = std::max(plugin_device.max_width, res.width);
                plugin_device.max_height = std::max(plugin_device.max_height, res.height);
            }
            for (int fr : stream.supported_framerates) {
                row.framerateLabels.append(QStringLiteral("%1 fps").arg(fr));
                plugin_device.max_framerate = std::max(plugin_device.max_framerate, static_cast<double>(fr));
            }
            for (const auto& fmt : stream.supported_formats) {
                row.formatLabels.append(QString::fromStdString(fmt));
            }
            rows.push_back(std::move(row));
        }
        if (source) {
            plugin_devices.push_back(std::move(plugin_device));
        }
    }

    for (const auto& source : sources) {
        auto source_devices = manager_.get_devices_for_source(source.source_id);
        plugin_devices.insert(plugin_devices.end(), source_devices.begin(), source_devices.end());
    }

    const int previous_count = cameraCount();
    camera_model_->replaceRows(std::move(rows));

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

    if (!config.streams.empty()) {
        current_encoder_name_ = QStringLiteral("H.264");
        current_bitrate_ = QStringLiteral("5.0 Mbps");
        emit encoderNameChanged();
        emit bitrateChanged();
    }

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
    stream_frame_counts_.clear();
    stream_drop_counts_.clear();
    last_metrics_push_ = std::chrono::steady_clock::now();
    metrics_timer_->start();
    emit isRecordingChanged();
    emit canStartRecordingChanged();
    emit recordButtonTextChanged();
    emit lastSessionIdChanged();
    return true;
}

void AppController::stopRecording() {
    if (!recording_) return;

    metrics_timer_->stop();
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
    current_encoder_name_ = QStringLiteral("—");
    current_bitrate_ = QStringLiteral("—");
    emit isRecordingChanged();
    emit canStartRecordingChanged();
    emit recordButtonTextChanged();
    emit totalFramesTextChanged();
    emit averageFpsTextChanged();
    emit bytesWrittenTextChanged();
    emit encoderNameChanged();
    emit bitrateChanged();
}

QVariantList AppController::preflightItems() {
    QVariantList items;

    {
        QVariantMap item;
        item["name"] = QStringLiteral("Camera Detection");
        bool has_cameras = camera_model_->rowCount() > 0;
        item["status"] = has_cameras ? QStringLiteral("pass") : QStringLiteral("fail");
        item["detail"] = has_cameras
            ? QStringLiteral("%1 camera(s) detected").arg(camera_model_->rowCount())
            : QStringLiteral("No cameras detected");
        items.append(item);
    }

    {
        QVariantMap item;
        item["name"] = QStringLiteral("Disk Space");
        pipeline::PreflightValidator validator;
        std::string out_dir = output_dir_.isEmpty() ? "." : output_dir_.toStdString();
        bool disk_ok = validator.check_disk_space(out_dir, 500 * 1024 * 1024);
        item["status"] = disk_ok ? QStringLiteral("pass") : QStringLiteral("fail");
        item["detail"] = disk_ok
            ? QStringLiteral("Sufficient disk space available")
            : QStringLiteral("Insufficient disk space for recording");
        items.append(item);
    }

    {
        QVariantMap item;
        item["name"] = QStringLiteral("Encoder Availability");
        bool encoder_ok = current_encoder_name_ != QStringLiteral("—") || camera_model_->rowCount() > 0;
        item["status"] = encoder_ok ? QStringLiteral("pass") : QStringLiteral("fail");
        item["detail"] = encoder_ok
            ? QStringLiteral("H.264 encoder available")
            : QStringLiteral("No H.264 encoder found");
        items.append(item);
    }

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
        item["apiVersion"] = static_cast<int>(src.plugin_api_version);
        item["deviceCount"] = static_cast<int>(src.device_ids.size());
        item["restartRequired"] = src.restart_required;
        item["canToggle"] = src.source_type != domain::PluginSourceType::BUNDLED && !recording_;
        item["canRemove"] = src.source_type == domain::PluginSourceType::LINKED && !recording_;
        item["canOpenSettings"] = true;
        item["statusMessage"] = QString::fromStdString(src.diagnostics_message);

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
            if (src.source_type == domain::PluginSourceType::BUNDLED) {
                pushLogEntry(log_entries_,
                    QStringLiteral("[WARN] Bundled plugin cannot be disabled: %1")
                        .arg(QString::fromStdString(src.source_id)));
                emit logEntriesChanged();
                return;
            }
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

bool AppController::removePlugin(const QString& pluginPath) {
    if (recording_) return false;

    auto sources = plugin_registry_.getSources();
    for (const auto& src : sources) {
        if (src.plugin_path == pluginPath.toStdString()) {
            if (src.source_type != domain::PluginSourceType::LINKED) {
                pushLogEntry(log_entries_,
                    QStringLiteral("[WARN] Bundled plugin cannot be removed: %1")
                        .arg(QString::fromStdString(src.source_id)));
                emit logEntriesChanged();
                return false;
            }
            bool ok = plugin_registry_.removeLinkedDirectory(src.plugin_path);
            if (ok) {
                pushLogEntry(log_entries_,
                    QStringLiteral("[INFO] Plugin removed: %1 (restart required)")
                        .arg(QString::fromStdString(src.source_id)));
                emit pluginsChanged();
            }
            return ok;
        }
    }
    return false;
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
            result["restartRequired"] = src.restart_required;
            result["statusMessage"] = QString::fromStdString(src.diagnostics_message);

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

            std::string stream_id = active.config.device_id + "_" +
                std::to_string(active.config.stream_index);

            pipeline::FrameData frame;
            frame.stream_id = stream_id;
            frame.data = bytes.data();
            frame.size = bytes.size();
            frame.width = active.stream->width();
            frame.height = active.stream->height();
            frame.pts = pts;
            frame.source_format = active.stream->pixel_format();

            if (pipeline_.push_frame(frame)) {
                total_frames_++;
                bytes_written_ += bytes.size();
                stream_frame_counts_[stream_id]++;
                stream_drop_counts_[stream_id] += frame.dropped_frame_count;
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

void AppController::pushLiveMetrics() {
    if (!recording_) return;

    auto now = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration<double>(now - last_metrics_push_).count();
    if (elapsed_sec < 0.5) return;
    last_metrics_push_ = now;

    for (auto& active : active_streams_) {
        std::string sid = active.config.device_id + "_" +
            std::to_string(active.config.stream_index);

        uint64_t frames = stream_frame_counts_[sid];
        double fps = elapsed_sec > 0 ? static_cast<double>(frames) / elapsed_sec : 0.0;
        uint64_t drops = stream_drop_counts_[sid];

        QString deviceId = QString::fromStdString(active.config.device_id);
        source_model_->updateDeviceMetrics(deviceId, fps, static_cast<int>(drops));

        stream_frame_counts_[sid] = 0;
    }
}

void AppController::setupCrashAlertHandler() {
    plugin_registry_.set_crash_alert_callback([this](const std::string& plugin_id) {
        QMetaObject::invokeMethod(this, [this, plugin_id] {
            handlePluginCrash(plugin_id);
        }, Qt::QueuedConnection);
    });
}

void AppController::handlePluginCrash(const std::string& pluginId) {
    QString pluginName = QString::fromStdString(pluginId);
    pushLogEntry(log_entries_,
        QStringLiteral("[WARN] Plugin crashed: %1").arg(pluginName));

    alert_model_->pushAlert(
        QStringLiteral("Plugin crash detected — recovering..."),
        pluginName, 1,
        QStringLiteral("crash_%1").arg(pluginName),
        true);

    emit pluginCrashAlert(pluginName);

    auto streams = plugin_registry_.get_streams_for_plugin(pluginId);
    for (const auto& stream_id : streams) {
        pipeline_.finalize_stream(stream_id);
    }

    auto result = plugin_registry_.handle_plugin_crash(pluginId);
    if (result.restart_succeeded) {
        alert_model_->dismissBySource(pluginName);
        pushLogEntry(log_entries_,
            QStringLiteral("[INFO] Plugin %1 restarted, starting reconnect recording")
                .arg(pluginName));
        int reconnect_idx = 1;
        for (const auto& stream_id : result.finalized_streams) {
            pipeline_.start_reconnect(stream_id, reconnect_idx);
        }
    } else {
        alert_model_->dismissBySource(pluginName);
        alert_model_->pushAlert(
            QStringLiteral("Plugin recovery failed — %1").arg(pluginName),
            pluginName, 2,
            QStringLiteral("crash_%1").arg(pluginName),
            false);
        pushLogEntry(log_entries_,
            QStringLiteral("[ERROR] Plugin %1 recovery failed")
                .arg(pluginName));
    }
}

void AppController::handleDeviceDisconnect(const std::string& streamId,
                                            const std::string& deviceName) {
    QString devName = QString::fromStdString(deviceName);
    pushLogEntry(log_entries_,
        QStringLiteral("[WARN] Device disconnected: %1 (stream: %2)")
            .arg(devName,
                 QString::fromStdString(streamId)));

    alert_model_->pushAlert(
        QStringLiteral("Device disconnected: %1 — check connection").arg(devName),
        devName, 1,
        QStringLiteral("disconnect_%1").arg(QString::fromStdString(streamId)),
        false);

    emit deviceDisconnected(devName);

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

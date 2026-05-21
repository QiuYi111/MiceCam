#include "PluginRegistryService.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fcntl.h>
#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace micecam::infrastructure {

namespace fs = std::filesystem;

static int default_shm_unlink(const std::string& name) {
#ifndef _WIN32
    return shm_unlink(name.c_str());
#else
    (void)name;
    return -1;
#endif
}

PluginRegistryService::PluginRegistryService(const std::string& bundled_plugins_dir,
                                             const std::string& config_dir,
                                             uint64_t stall_timeout_ms)
    : bundled_plugins_dir_(bundled_plugins_dir)
    , linked_config_(config_dir + "/linked_plugins.json")
    , shm_unlink_fn_(default_shm_unlink)
    , stall_timeout_ms_(stall_timeout_ms) {}

bool PluginRegistryService::initialize() {
    linked_config_.load();

    scanBundledDirectory();
    scanLinkedDirectories();

    monitor_ = std::make_unique<StreamLivenessMonitor>(stall_timeout_ms_);

    monitor_->set_stall_callback([this](const std::string& stream_id,
                                         const std::string& plugin_id,
                                         uint64_t stall_duration_ms,
                                         int stall_count) {
        {
            std::lock_guard<std::mutex> lock(registry_mutex_);
            auto it = plugin_streams_.find(plugin_id);
            if (it == plugin_streams_.end()) return;
            auto& vec = it->second;
            if (std::find(vec.begin(), vec.end(), stream_id) == vec.end()) return;
        }

        if (!notify_stall_fn_) return;

        auto result = notify_stall_fn_(stream_id, plugin_id, stall_duration_ms);

        if (!result.acknowledged) {
            spdlog::warn("Stall not acknowledged for stream {}, finalizing", stream_id);
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
                auto it = plugin_streams_.find(plugin_id);
                if (it != plugin_streams_.end()) {
                    auto& vec = it->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), stream_id), vec.end());
                }
            }
            if (crash_alert_cb_) crash_alert_cb_(plugin_id);
            return;
        }

        if (!result.recoverable) {
            spdlog::warn("Stall unrecoverable for stream {}, finalizing", stream_id);
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
                auto it = plugin_streams_.find(plugin_id);
                if (it != plugin_streams_.end()) {
                    auto& vec = it->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), stream_id), vec.end());
                }
            }
            if (crash_alert_cb_) crash_alert_cb_(plugin_id);
            return;
        }

        if (stall_count >= max_stall_retries_) {
            spdlog::warn("Stall escalation for stream {} after {} retries, finalizing",
                         stream_id, stall_count);
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
                auto it = plugin_streams_.find(plugin_id);
                if (it != plugin_streams_.end()) {
                    auto& vec = it->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), stream_id), vec.end());
                }
            }
            if (crash_alert_cb_) crash_alert_cb_(plugin_id);
            return;
        }

        spdlog::info("Stall recoverable for stream {}, attempt {}/{}",
                     stream_id, stall_count, max_stall_retries_);
    });

    monitor_->set_all_stalled_callback([this](const std::string& plugin_id) {
        spdlog::warn("All streams stalled for plugin {}, triggering crash recovery", plugin_id);
        if (crash_alert_cb_) crash_alert_cb_(plugin_id);
        handle_plugin_crash(plugin_id);
    });

    monitor_->start();

    pending_restart_ = false;
    initialized_ = true;
    return true;
}

std::string PluginRegistryService::manifestPathFor(const std::string& dir_path) const {
    return dir_path + "/plugin.json";
}

void PluginRegistryService::registerPlugin(const std::string& dir_path,
                                           domain::PluginSourceType source_type) {
    auto manifest_path = manifestPathFor(dir_path);
    if (!fs::exists(manifest_path)) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        "plugin.json not found at: " + manifest_path);
        return;
    }

    std::ifstream file(manifest_path);
    if (!file.is_open()) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        "Cannot open plugin.json at: " + manifest_path);
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        std::string("JSON parse error: ") + e.what());
        return;
    }

    domain::PluginManifest manifest;
    try {
        manifest = domain::PluginManifest::from_json(j);
    } catch (const std::exception& e) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        std::string("Manifest parse error: ") + e.what());
        return;
    }

    auto errors = manifest.validate();
    if (!errors.empty()) {
        std::string all_errors;
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0) all_errors += "; ";
            all_errors += errors[i];
        }
        recordDiagnostic(manifest.id, "MANIFEST_PARSE_ERROR", all_errors);
    }

    domain::PluginDescriptor desc;
    desc.id = manifest.id;
    desc.name = manifest.name;
    desc.version = manifest.version;
    desc.api_version = manifest.plugin_api_version;
    desc.path = dir_path;
    desc.type = manifest.preferred_process_model;
    desc.source_type = source_type;
    desc.enabled = true;

    plugins_.push_back(std::move(desc));
}

void PluginRegistryService::recordDiagnostic(const std::string& id,
                                             const std::string& code,
                                             const std::string& message) {
    diagnostics_.push_back({id, code, message});
}

void PluginRegistryService::scanBundledDirectory() {
    if (!fs::exists(bundled_plugins_dir_) || !fs::is_directory(bundled_plugins_dir_)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(bundled_plugins_dir_)) {
        if (!entry.is_directory()) continue;

        auto plugin_json = entry.path() / "plugin.json";
        if (fs::exists(plugin_json)) {
            registerPlugin(entry.path().string(), domain::PluginSourceType::BUNDLED);
        }
    }
}

void PluginRegistryService::scanLinkedDirectories() {
    for (const auto& linked_path : linked_config_.paths()) {
        if (!fs::exists(linked_path) || !fs::is_directory(linked_path)) {
            recordDiagnostic(linked_path, "CONFIG_INVALID",
                            "Linked plugin directory does not exist: " + linked_path);
            continue;
        }

        auto plugin_json = fs::path(linked_path) / "plugin.json";
        if (!fs::exists(plugin_json)) {
            recordDiagnostic(linked_path, "MANIFEST_PARSE_ERROR",
                            "plugin.json not found in linked directory: " + linked_path);
            continue;
        }

        registerPlugin(linked_path, domain::PluginSourceType::LINKED);
    }
}

bool PluginRegistryService::addLinkedDirectory(const std::string& path) {
    auto abs_path = fs::absolute(path).string();

    auto plugin_json = fs::path(abs_path) / "plugin.json";
    if (!fs::exists(plugin_json)) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        "plugin.json not found at: " + plugin_json.string());
        return false;
    }

    std::ifstream file(plugin_json);
    if (!file.is_open()) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        "Cannot open: " + plugin_json.string());
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        std::string("JSON parse error: ") + e.what());
        return false;
    }

    domain::PluginManifest manifest;
    try {
        manifest = domain::PluginManifest::from_json(j);
    } catch (const std::exception& e) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        std::string("Manifest parse error: ") + e.what());
        return false;
    }

    auto errors = manifest.validate();
    if (!errors.empty()) {
        std::string all_errors;
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0) all_errors += "; ";
            all_errors += errors[i];
        }
        recordDiagnostic(manifest.id, "MANIFEST_PARSE_ERROR", all_errors);
        return false;
    }

    linked_config_.add(abs_path);
    linked_config_.save();

    registerPlugin(abs_path, domain::PluginSourceType::LINKED);
    pending_restart_ = true;
    return true;
}

bool PluginRegistryService::removeLinkedDirectory(const std::string& path) {
    linked_config_.remove(path);
    linked_config_.save();

    // Remove from in-memory registry immediately
    plugins_.erase(
        std::remove_if(plugins_.begin(), plugins_.end(),
                       [&path](const domain::PluginDescriptor& p) {
                           return p.path == path;
                       }),
        plugins_.end());

    pending_restart_ = true;
    return true;
}

void PluginRegistryService::enablePlugin(const std::string& id) {
    for (auto& p : plugins_) {
        if (p.id == id) {
            if (!p.enabled) {
                p.enabled = true;
                pending_restart_ = true;
            }
            return;
        }
    }
}

void PluginRegistryService::disablePlugin(const std::string& id) {
    for (auto& p : plugins_) {
        if (p.id == id) {
            if (p.enabled) {
                p.enabled = false;
                pending_restart_ = true;
            }
            return;
        }
    }
}

std::vector<domain::PluginDescriptor> PluginRegistryService::discoverAll() {
    return getPlugins();
}

bool PluginRegistryService::isPendingRestart() const {
    return pending_restart_;
}

std::vector<domain::PluginDescriptor> PluginRegistryService::getPlugins() const {
    std::vector<domain::PluginDescriptor> enabled;
    for (const auto& p : plugins_) {
        if (p.enabled) {
            enabled.push_back(p);
        }
    }
    return enabled;
}

std::vector<domain::PluginSource> PluginRegistryService::getSources() const {
    std::vector<domain::PluginSource> sources;
    for (const auto& p : plugins_) {
        domain::PluginSource src;
        src.source_id = p.id;
        src.source_name = p.name;
        src.source_type = p.source_type;
        src.plugin_path = p.path;
        src.plugin_version = p.version;
        src.plugin_api_version = p.api_version;
        src.enabled = p.enabled;
        src.diagnostics_state = domain::PluginDiagnosticsState::OK;
        src.restart_required = pending_restart_;

        for (const auto& d : diagnostics_) {
            if (d.plugin_id == p.id) {
                src.diagnostics_state = domain::PluginDiagnosticsState::ERROR;
                src.diagnostics_message = d.message;
                break;
            }
        }
        if (!p.enabled) {
            src.diagnostics_state = domain::PluginDiagnosticsState::DISABLED;
            src.diagnostics_message = "Plugin disabled";
        }

        sources.push_back(std::move(src));
    }
    return sources;
}

const std::vector<PluginRegistryService::Diagnostics>&
PluginRegistryService::getDiagnostics() const {
    return diagnostics_;
}

void PluginRegistryService::register_stream(const std::string& plugin_id,
                                             const std::string& stream_id) {
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        plugin_streams_[plugin_id].push_back(stream_id);
    }
    if (monitor_) {
        monitor_->register_stream(stream_id, plugin_id);
    }
}

void PluginRegistryService::unregister_stream(const std::string& plugin_id,
                                               const std::string& stream_id) {
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        auto it = plugin_streams_.find(plugin_id);
        if (it != plugin_streams_.end()) {
            auto& vec = it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), stream_id), vec.end());
        }
    }
    if (monitor_) {
        monitor_->unregister_stream(stream_id);
    }
}

std::vector<std::string> PluginRegistryService::get_streams_for_plugin(
        const std::string& plugin_id) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = plugin_streams_.find(plugin_id);
    if (it != plugin_streams_.end()) {
        return it->second;
    }
    return {};
}

void PluginRegistryService::register_shm(const std::string& plugin_id,
                                          const std::string& shm_name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    plugin_shm_names_[plugin_id].push_back(shm_name);
}

void PluginRegistryService::unregister_shm(const std::string& plugin_id,
                                            const std::string& shm_name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = plugin_shm_names_.find(plugin_id);
    if (it != plugin_shm_names_.end()) {
        auto& vec = it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), shm_name), vec.end());
    }
}

std::vector<std::string> PluginRegistryService::get_shm_names_for_plugin(
        const std::string& plugin_id) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = plugin_shm_names_.find(plugin_id);
    if (it != plugin_shm_names_.end()) {
        return it->second;
    }
    return {};
}

void PluginRegistryService::set_crash_alert_callback(CrashAlertCallback cb) {
    crash_alert_cb_ = std::move(cb);
}

void PluginRegistryService::set_shm_unlink_fn(ShmUnlinkFn fn) {
    shm_unlink_fn_ = std::move(fn);
}

void PluginRegistryService::set_restart_fn(
        std::function<bool(const std::string& plugin_id)> fn) {
    restart_fn_ = std::move(fn);
}

void PluginRegistryService::set_notify_stall_fn(NotifyStallFn fn) {
    notify_stall_fn_ = std::move(fn);
}

StreamLivenessMonitor* PluginRegistryService::get_liveness_monitor() const {
    return monitor_.get();
}

void PluginRegistryService::detect_channel_failure(const std::string& plugin_id) {
    spdlog::warn("Plugin {} crashed, initiating recovery", plugin_id);

    if (crash_alert_cb_) {
        crash_alert_cb_(plugin_id);
    }

    handle_plugin_crash(plugin_id);
}

CrashRecoveryResult PluginRegistryService::handle_plugin_crash(
        const std::string& plugin_id) {
    CrashRecoveryResult result;
    result.plugin_id = plugin_id;

    std::lock_guard<std::mutex> lock(registry_mutex_);

    auto streams = plugin_streams_.find(plugin_id) != plugin_streams_.end()
                   ? plugin_streams_[plugin_id] : std::vector<std::string>();

    for (const auto& stream_id : streams) {
        spdlog::info("Finalized stream {} after plugin crash", stream_id);
        result.finalized_streams.push_back(stream_id);
    }

    auto shm_names = plugin_shm_names_.find(plugin_id) != plugin_shm_names_.end()
                     ? plugin_shm_names_[plugin_id] : std::vector<std::string>();

    for (const auto& shm_name : shm_names) {
        if (shm_unlink_fn_) {
            shm_unlink_fn_(shm_name);
            spdlog::info("Cleaned up shared memory for plugin {}", plugin_id);
        }
        result.cleaned_shm_names.push_back(shm_name);
    }

    plugin_shm_names_.erase(plugin_id);

    bool restarted = false;
    if (restart_fn_) {
        for (int attempt = 0; attempt < max_restart_retries_; ++attempt) {
            if (restart_fn_(plugin_id)) {
                restarted = true;
                break;
            }
        }
    }

    result.restart_succeeded = restarted;

    if (restarted) {
        spdlog::info("Plugin {} restarted successfully", plugin_id);
    } else {
        spdlog::error("Plugin {} restart failed, recording stopped", plugin_id);
        plugin_streams_.erase(plugin_id);
    }

    return result;
}

} // namespace micecam::infrastructure

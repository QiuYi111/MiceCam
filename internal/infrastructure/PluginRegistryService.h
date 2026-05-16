#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "domain/PluginDescriptor.h"
#include "domain/PluginManifest.h"
#include "domain/PluginSource.h"
#include "infrastructure/LinkedPluginConfig.h"

namespace micecam::infrastructure {

struct CrashRecoveryResult {
    std::string plugin_id;
    bool restart_succeeded = false;
    std::vector<std::string> finalized_streams;
    std::vector<std::string> cleaned_shm_names;
};

class PluginRegistryService {
public:
    struct Diagnostics {
        std::string plugin_id;
        std::string error_code;
        std::string message;
    };

    using CrashAlertCallback = std::function<void(const std::string& plugin_id)>;
    using ShmUnlinkFn = std::function<int(const std::string&)>;

    PluginRegistryService(const std::string& bundled_plugins_dir,
                         const std::string& config_dir);

    bool initialize();
    bool addLinkedDirectory(const std::string& path);
    bool removeLinkedDirectory(const std::string& path);
    void enablePlugin(const std::string& id);
    void disablePlugin(const std::string& id);
    std::vector<domain::PluginDescriptor> discoverAll();
    bool isPendingRestart() const;
    std::vector<domain::PluginDescriptor> getPlugins() const;
    std::vector<domain::PluginSource> getSources() const;
    const std::vector<Diagnostics>& getDiagnostics() const;

    void register_stream(const std::string& plugin_id, const std::string& stream_id);
    void unregister_stream(const std::string& plugin_id, const std::string& stream_id);
    std::vector<std::string> get_streams_for_plugin(const std::string& plugin_id) const;

    void register_shm(const std::string& plugin_id, const std::string& shm_name);
    void unregister_shm(const std::string& plugin_id, const std::string& shm_name);
    std::vector<std::string> get_shm_names_for_plugin(const std::string& plugin_id) const;

    void set_crash_alert_callback(CrashAlertCallback cb);
    void set_shm_unlink_fn(ShmUnlinkFn fn);

    void detect_channel_failure(const std::string& plugin_id);
    CrashRecoveryResult handle_plugin_crash(const std::string& plugin_id);

    void set_restart_fn(std::function<bool(const std::string& plugin_id)> fn);

private:
    std::string bundled_plugins_dir_;
    LinkedPluginConfig linked_config_;
    std::vector<domain::PluginDescriptor> plugins_;
    std::vector<Diagnostics> diagnostics_;
    bool pending_restart_ = false;
    bool initialized_ = false;

    std::unordered_map<std::string, std::vector<std::string>> plugin_streams_;
    std::unordered_map<std::string, std::vector<std::string>> plugin_shm_names_;
    mutable std::unordered_map<std::string, std::mutex> plugin_recovery_mutexes_;
    mutable std::mutex registry_mutex_;

    CrashAlertCallback crash_alert_cb_;
    ShmUnlinkFn shm_unlink_fn_;
    std::function<bool(const std::string&)> restart_fn_;
    int max_restart_retries_ = 3;

    void scanBundledDirectory();
    void scanLinkedDirectories();
    void registerPlugin(const std::string& dir_path,
                        domain::PluginSourceType source_type);
    void recordDiagnostic(const std::string& id, const std::string& code,
                         const std::string& message);
    std::string manifestPathFor(const std::string& dir_path) const;
};

} // namespace micecam::infrastructure

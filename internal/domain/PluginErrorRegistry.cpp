#include "PluginErrorRegistry.h"

#include <stdexcept>

namespace micecam::domain {

const std::unordered_map<PluginErrorCode, ErrorMeta>& PluginErrorRegistry::entries() {
    static const std::unordered_map<PluginErrorCode, ErrorMeta> map = {
        {PluginErrorCode::MANIFEST_PARSE_ERROR,
         {ErrorSeverity::ERROR, true, "Plugin manifest could not be parsed",
          "Check that plugin.json exists and is valid JSON",
          RecoveryAction::NOTIFY_USER}},
        {PluginErrorCode::HANDSHAKE_FAILED,
         {ErrorSeverity::ERROR, true, "Plugin handshake failed",
          "Verify plugin version and API compatibility",
          RecoveryAction::RETRY}},
        {PluginErrorCode::DEVICE_UNAVAILABLE,
         {ErrorSeverity::WARN, true, "Device is unavailable",
          "Check device connection and power",
          RecoveryAction::RETRY}},
        {PluginErrorCode::DEVICE_BUSY,
         {ErrorSeverity::WARN, true, "Device is in use by another process",
          "Close other applications using this device",
          RecoveryAction::RETRY}},
        {PluginErrorCode::ENUMERATION_FAILED,
         {ErrorSeverity::ERROR, true, "Device enumeration failed",
          "Check that the device driver is installed and working",
          RecoveryAction::RETRY}},
        {PluginErrorCode::STREAM_OPEN_FAILED,
         {ErrorSeverity::ERROR, true, "Failed to open stream",
          "Verify device is still connected and supported",
          RecoveryAction::RETRY}},
        {PluginErrorCode::STREAM_WRITE_FAILED,
         {ErrorSeverity::ERROR, true, "Failed to write to stream ring buffer",
          "Check that the shared memory region is accessible",
          RecoveryAction::RESTART_STREAM}},
        {PluginErrorCode::SHM_ALLOC_FAILED,
         {ErrorSeverity::FATAL, false, "Shared memory allocation failed",
          "Check system memory availability and limits",
          RecoveryAction::NOTIFY_USER}},
        {PluginErrorCode::CAPABILITY_MISMATCH,
         {ErrorSeverity::WARN, true, "Requested capability not supported by device",
          "Select a different device or adjust stream configuration",
          RecoveryAction::FALLBACK}},
        {PluginErrorCode::CONFIG_INVALID,
         {ErrorSeverity::ERROR, true, "Configuration is invalid",
          "Review configuration values against the plugin schema",
          RecoveryAction::NOTIFY_USER}},
        {PluginErrorCode::CONFIG_WRITE_FAILED,
         {ErrorSeverity::ERROR, true, "Failed to apply configuration",
          "Verify write permissions on the config store",
          RecoveryAction::NOTIFY_USER}},
        {PluginErrorCode::PLUGIN_TIMEOUT,
         {ErrorSeverity::ERROR, true, "Plugin process timed out",
          "Check plugin health and restart if necessary",
          RecoveryAction::RESTART_PLUGIN}},
        {PluginErrorCode::PLUGIN_CRASH,
         {ErrorSeverity::FATAL, true, "Plugin process crashed",
          "Restart the plugin process",
          RecoveryAction::RESTART_PLUGIN}},
        {PluginErrorCode::BACKPRESSURE,
         {ErrorSeverity::WARN, true, "Ring buffer backpressure detected",
          "Increase ring buffer size or reduce stream throughput",
          RecoveryAction::RETRY}},
        {PluginErrorCode::DISK_FULL,
         {ErrorSeverity::ERROR, true, "Disk is full",
          "Free up disk space on the recording target",
          RecoveryAction::NOTIFY_USER}},
        {PluginErrorCode::EXCLUSIVE_CONFLICT,
         {ErrorSeverity::WARN, true, "Exclusive resource conflict detected",
          "A device using the same hardware resource is already active",
          RecoveryAction::RETRY}},
        {PluginErrorCode::SDK_MISSING,
         {ErrorSeverity::FATAL, false, "Required device SDK is not installed",
          "Install the required SDK for this plugin",
          RecoveryAction::NOTIFY_USER}}};
    return map;
}

const ErrorMeta& PluginErrorRegistry::get(PluginErrorCode code) {
    const auto& map = entries();
    auto it = map.find(code);
    if (it == map.end()) {
        throw std::runtime_error("Unknown error code");
    }
    return it->second;
}

} // namespace micecam::domain

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace micecam::domain {

enum class ErrorSeverity { INFO = 0, WARN = 1, ERROR = 2, FATAL = 3 };

enum class RecoveryAction { NO_RECOVERY = 0, RETRY = 1, RESTART_PLUGIN = 2, RESTART_STREAM = 3, NOTIFY_USER = 4, FALLBACK = 5 };

enum class PluginErrorCode {
    MANIFEST_PARSE_ERROR,
    HANDSHAKE_FAILED,
    DEVICE_UNAVAILABLE,
    DEVICE_BUSY,
    ENUMERATION_FAILED,
    STREAM_OPEN_FAILED,
    STREAM_WRITE_FAILED,
    SHM_ALLOC_FAILED,
    CAPABILITY_MISMATCH,
    CONFIG_INVALID,
    CONFIG_WRITE_FAILED,
    PLUGIN_TIMEOUT,
    PLUGIN_CRASH,
    BACKPRESSURE,
    DISK_FULL,
    EXCLUSIVE_CONFLICT,
    SDK_MISSING
};

struct ErrorMeta {
    ErrorSeverity severity;
    bool is_recoverable;
    std::string_view default_user_message;
    std::string_view default_suggested_action;
    RecoveryAction default_recovery_action;
};

class PluginErrorRegistry {
public:
    static const std::unordered_map<PluginErrorCode, ErrorMeta>& entries();
    static const ErrorMeta& get(PluginErrorCode code);
};

} // namespace micecam::domain

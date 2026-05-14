#pragma once

#include <cstdint>
#include <string>

namespace micecam::domain {

enum class AlertSeverity { YELLOW, RED };

enum class AlertType {
    CAMERA_DISCONNECT,
    CAMERA_RECONNECT,
    HIGH_DROP_RATE,
    ENCODE_STALL,
    ENCODER_FALLBACK,
    DISK_FULL,
    PIPELINE_STALL
};

struct AlertRecord {
    uint64_t timestamp_ns = 0;
    AlertSeverity severity = AlertSeverity::YELLOW;
    AlertType type = AlertType::CAMERA_DISCONNECT;
    std::string stream_id;
    std::string message;
};

} // namespace micecam::domain

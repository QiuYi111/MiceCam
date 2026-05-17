#include "FeishuWebhook.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>

namespace micecam::infrastructure {

FeishuWebhook::FeishuWebhook() = default;

FeishuWebhook::~FeishuWebhook() = default;

void FeishuWebhook::configure(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    url_ = url;
}

void FeishuWebhook::on_alert(const domain::AlertRecord& alert) {
    std::string local_url;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local_url = url_;
    }

    if (local_url.empty()) return;

    std::string payload = format_payload(alert);

    for (int attempt = 0; attempt < 2; attempt++) {
        if (send(payload)) break;
        if (attempt == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

std::string FeishuWebhook::format_payload(const domain::AlertRecord& alert) const {
    std::string type_str;
    switch (alert.type) {
        case domain::AlertType::CAMERA_DISCONNECT: type_str = "CAMERA_DISCONNECT"; break;
        case domain::AlertType::CAMERA_RECONNECT: type_str = "CAMERA_RECONNECT"; break;
        case domain::AlertType::HIGH_DROP_RATE: type_str = "HIGH_DROP_RATE"; break;
        case domain::AlertType::ENCODE_STALL: type_str = "ENCODE_STALL"; break;
        case domain::AlertType::ENCODER_FALLBACK: type_str = "ENCODER_FALLBACK"; break;
        case domain::AlertType::DISK_FULL: type_str = "DISK_FULL"; break;
        case domain::AlertType::PIPELINE_STALL: type_str = "PIPELINE_STALL"; break;
    }

    std::string severity_str = (alert.severity == domain::AlertSeverity::RED) ? "RED" : "YELLOW";

    std::string text = "[MiceCam] ALERT: " + type_str + " — " + alert.message +
                       " (stream: " + alert.stream_id + ", severity: " + severity_str + ")";

    nlohmann::json payload;
    payload["msg_type"] = "text";
    payload["content"]["text"] = text;
    return payload.dump();
}

bool FeishuWebhook::send(const std::string& /*payload*/) {
    return true;
}

} // namespace micecam::infrastructure

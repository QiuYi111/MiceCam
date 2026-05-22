#include "FeishuWebhook.h"

#include <cstdio>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace micecam::infrastructure {

FeishuWebhook::FeishuWebhook() {}

FeishuWebhook::~FeishuWebhook() {}

void FeishuWebhook::configure(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    url_ = url;
}

void FeishuWebhook::on_alert(const domain::AlertRecord& alert) {
    send(format_payload(alert));
}

std::string FeishuWebhook::format_payload(const domain::AlertRecord& alert) const {
    nlohmann::json payload;
    payload["msg_type"] = "interactive";
    payload["card"]["header"]["title"]["tag"] = "plain_text";
    payload["card"]["header"]["title"]["content"] = "[MiceCam Alert] " + alert.message;
    payload["card"]["header"]["template"] =
        alert.severity == domain::AlertSeverity::RED ? "red" : "yellow";

    std::string typeStr;
    switch (alert.type) {
        case domain::AlertType::CAMERA_DISCONNECT: typeStr = "Camera Disconnect"; break;
        case domain::AlertType::HIGH_DROP_RATE:    typeStr = "High Drop Rate"; break;
        case domain::AlertType::ENCODE_STALL:      typeStr = "Encoder Stall"; break;
        case domain::AlertType::ENCODER_FALLBACK:  typeStr = "Encoder Fallback"; break;
        case domain::AlertType::DISK_FULL:         typeStr = "Disk Full"; break;
        case domain::AlertType::PIPELINE_STALL:    typeStr = "Pipeline Stall"; break;
        default:                                   typeStr = "Unknown"; break;
    }

    nlohmann::json fields;
    fields.push_back({{"is_short", true}, {"text", {{"tag", "lark_md"}, {"content", "**Type:** " + typeStr}}}});
    fields.push_back({{"is_short", true}, {"text", {{"tag", "lark_md"}, {"content", "**Stream:** " + alert.stream_id}}}});

    payload["card"]["elements"] = {{{"tag", "div"}, {"fields", fields}}};
    return payload.dump();
}

bool FeishuWebhook::send(const std::string& payload) {
    std::string url;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        url = url_;
    }
    if (url.empty()) {
        spdlog::warn("FeishuWebhook: no URL configured, skipping send");
        return false;
    }

    // Use curl via popen for cross-platform HTTP POST without adding libcurl dependency
    std::string cmd = "curl -s -X POST -H 'Content-Type: application/json' -d '" + payload + "' '" + url + "' --connect-timeout 5 --max-time 10";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        spdlog::error("FeishuWebhook: failed to execute curl");
        return false;
    }

    char buf[256];
    std::string response;
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        response += buf;
    }
    int rc = pclose(pipe);

    if (rc != 0) {
        spdlog::error("FeishuWebhook: curl failed with code {}, response: {}", rc, response);
        return false;
    }

    spdlog::info("FeishuWebhook: sent alert, response: {}", response.substr(0, 100));
    return true;
}

} // namespace micecam::infrastructure

#include <gtest/gtest.h>

#include "domain/AlertRecord.h"
#include "infrastructure/FeishuWebhook.h"

using namespace micecam;

TEST(FeishuWebhook, FormatPayloadValidJson) {
    infrastructure::FeishuWebhook webhook;
    webhook.configure("https://example.com/webhook");

    domain::AlertRecord alert;
    alert.type = domain::AlertType::PIPELINE_STALL;
    alert.severity = domain::AlertSeverity::RED;
    alert.stream_id = "cam_a";
    alert.message = "Pipeline stalled for 5 seconds";

    std::string payload = webhook.format_payload(alert);
    EXPECT_FALSE(payload.empty());
    EXPECT_NE(payload.find("msg_type"), std::string::npos);
    EXPECT_NE(payload.find("\"text\""), std::string::npos);
    EXPECT_NE(payload.find("Pipeline Stall"), std::string::npos);
}

TEST(FeishuWebhook, ObserverInterfaceCalled) {
    infrastructure::FeishuWebhook webhook;
    webhook.configure("https://example.com/webhook");

    domain::AlertRecord alert;
    alert.type = domain::AlertType::CAMERA_DISCONNECT;
    alert.severity = domain::AlertSeverity::RED;
    alert.stream_id = "cam_a";
    alert.message = "Camera disconnected";

    webhook.on_alert(alert);
}

TEST(FeishuWebhook, EmptyUrlDoesNotThrow) {
    infrastructure::FeishuWebhook webhook;

    domain::AlertRecord alert;
    alert.type = domain::AlertType::DISK_FULL;

    webhook.on_alert(alert);
}

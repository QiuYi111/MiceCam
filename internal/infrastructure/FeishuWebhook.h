#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "api/micecam/WatchdogObserver.h"
#include "domain/AlertRecord.h"

namespace micecam::infrastructure {

class FeishuWebhook : public api::WatchdogObserver {
public:
    FeishuWebhook();
    ~FeishuWebhook() override;

    void configure(const std::string& url);
    void on_alert(const domain::AlertRecord& alert) override;
    std::string format_payload(const domain::AlertRecord& alert) const;

private:
    bool send(const std::string& payload);

    std::string url_;
    std::mutex mutex_;
};

} // namespace micecam::infrastructure

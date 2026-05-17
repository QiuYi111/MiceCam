#pragma once

namespace micecam::domain {
struct AlertRecord;
}

namespace micecam::api {

class WatchdogObserver {
public:
    virtual ~WatchdogObserver() = default;

    virtual void on_alert(const domain::AlertRecord& alert) = 0;
};

} // namespace micecam::api

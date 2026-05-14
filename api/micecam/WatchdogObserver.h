#pragma once

#include "internal/domain/AlertRecord.h"

namespace micecam::api {

class WatchdogObserver {
public:
    virtual ~WatchdogObserver() = default;

    virtual void on_alert(const domain::AlertRecord& alert) = 0;
};

} // namespace micecam::api

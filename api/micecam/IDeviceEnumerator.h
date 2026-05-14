#pragma once

#include <vector>

#include "internal/domain/DeviceInfo.h"

namespace micecam::api {

class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;

    virtual std::vector<domain::DeviceInfo> enumerate() = 0;
};

} // namespace micecam::api

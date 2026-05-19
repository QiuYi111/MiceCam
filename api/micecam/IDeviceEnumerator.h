#pragma once

#include <vector>

namespace micecam::domain {
struct DeviceInfo;
}

namespace micecam::api {

class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;

    virtual std::vector<domain::DeviceInfo> enumerate() = 0;
};

} // namespace micecam::api

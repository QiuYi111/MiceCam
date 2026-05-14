#pragma once

#include <optional>
#include <vector>

namespace micecam {

template <typename DeviceInfoT>
std::optional<DeviceInfoT> resolve_oak_device_info(
    const std::vector<DeviceInfoT>& available_devices,
    int device_id
) {
    if (device_id < 0 || device_id >= static_cast<int>(available_devices.size())) {
        return std::nullopt;
    }

    return available_devices.at(device_id);
}

}  // namespace micecam

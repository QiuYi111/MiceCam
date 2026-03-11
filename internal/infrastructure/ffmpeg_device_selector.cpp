#include "infrastructure/ffmpeg_device_selector.h"

namespace micecam {

std::optional<std::string> resolve_ffmpeg_input_device_name(
    const std::vector<std::string>& device_names,
    int device_id,
    const std::string& input_prefix
) {
    if (device_id < 0 || device_id >= static_cast<int>(device_names.size())) {
        return std::nullopt;
    }

    const std::string& selected_name = device_names.at(device_id);
    if (input_prefix.empty() || selected_name.rfind(input_prefix, 0) == 0) {
        return selected_name;
    }

    return input_prefix + selected_name;
}

}  // namespace micecam

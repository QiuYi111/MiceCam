#pragma once

#include <optional>
#include <string>
#include <vector>

namespace micecam {

std::optional<std::string> resolve_ffmpeg_input_device_name(
    const std::vector<std::string>& device_names,
    int device_id,
    const std::string& input_prefix,
    const std::optional<std::string>& preferred_device_name
);

}  // namespace micecam

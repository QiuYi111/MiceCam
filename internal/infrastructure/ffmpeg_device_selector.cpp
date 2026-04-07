#include "infrastructure/ffmpeg_device_selector.h"

#include <algorithm>
#include <cctype>

namespace micecam {

namespace {

std::string trim_copy(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::optional<std::string> prefix_input_name(const std::string& selected_name, const std::string& input_prefix) {
    const std::string trimmed_name = trim_copy(selected_name);
    if (trimmed_name.empty()) {
        return std::nullopt;
    }

    if (input_prefix.empty() || trimmed_name.rfind(input_prefix, 0) == 0) {
        return trimmed_name;
    }

    return input_prefix + trimmed_name;
}

}  // namespace

std::optional<std::string> resolve_ffmpeg_input_device_name(
    const std::vector<std::string>& device_names,
    int device_id,
    const std::string& input_prefix,
    const std::optional<std::string>& preferred_device_name
) {
    if (preferred_device_name.has_value()) {
        if (const auto preferred_input = prefix_input_name(*preferred_device_name, input_prefix)) {
            return preferred_input;
        }
    }

    if (device_id < 0 || device_id >= static_cast<int>(device_names.size())) {
        return std::nullopt;
    }

    return prefix_input_name(device_names.at(device_id), input_prefix);
}

}  // namespace micecam

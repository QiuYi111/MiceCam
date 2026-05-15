#include "AVFoundationEnumerator.h"

#include <cstdio>
#include <cstring>
#include <array>
#include <memory>
#include <regex>
#include <set>

#include <spdlog/spdlog.h>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

namespace micecam::plugin {

AVFoundationEnumerator::AVFoundationEnumerator() {
    avdevice_register_all();
#if defined(__APPLE__) && TARGET_OS_MAC
    available_ = true;
#endif
}

std::vector<EnumeratedDevice> AVFoundationEnumerator::enumerate() const {
    std::vector<EnumeratedDevice> result;
    if (!available_) return result;

#if defined(__APPLE__) && TARGET_OS_MAC
    const AVInputFormat* fmt = av_find_input_format("avfoundation");
    if (!fmt) {
        spdlog::warn("avfoundation input format not found in FFmpeg");
        return result;
    }

    AVDeviceInfoList* dev_list = nullptr;
    int ret = avdevice_list_input_sources(fmt, nullptr, nullptr, &dev_list);
    if (ret < 0 || !dev_list) {
        spdlog::warn("avdevice_list_input_sources failed: {}", ret);
        return result;
    }

    for (int i = 0; i < dev_list->nb_devices; i++) {
        auto* dev = dev_list->devices[i];
        if (!dev) continue;

        const char* name = dev->device_name ? dev->device_name : "Unknown Camera";

        std::string raw_name(name);

        // Determine if this is a video or audio device
        // AVFoundation indexes: 0=FaceTime Camera (video), 1=Microphone (audio)
        // Video devices typically have "Camera", "Capture", "Continuity" in the name
        bool is_video = true;
        std::string lower_name = raw_name;
        for (auto& c : lower_name) c = static_cast<char>(std::tolower(c));
        if (lower_name.find("microphone") != std::string::npos ||
            lower_name.find("built-in microphone") != std::string::npos ||
            lower_name.find("aggregate device") != std::string::npos ||
            lower_name.find("soundflower") != std::string::npos) {
            is_video = false;
        }

        if (!is_video) continue;

        EnumeratedDevice dev_info;
        dev_info.device_id = std::to_string(i);
        dev_info.display_name = raw_name;
        // Create a URL-safe persistent id from the device name
        std::string persistent = raw_name;
        for (auto& c : persistent) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
                c = '_';
            }
        }
        dev_info.persistent_id = persistent;
        dev_info.has_persistent_id = true;
        dev_info.vendor = "Apple";
        dev_info.serial = "";
        dev_info.max_width = 1920;
        dev_info.max_height = 1080;
        dev_info.max_framerate = 30.0;

        // Try to infer better capabilities based on device type
        if (lower_name.find("continuity") != std::string::npos) {
            dev_info.max_width = 3840;
            dev_info.max_height = 2160;
            dev_info.max_framerate = 60.0;
        } else if (lower_name.find("iphone") != std::string::npos ||
                   lower_name.find("ipad") != std::string::npos) {
            dev_info.max_width = 3840;
            dev_info.max_height = 2160;
            dev_info.max_framerate = 60.0;
        }

        result.push_back(std::move(dev_info));
    }

    avdevice_free_list_devices(&dev_list);
#endif

    return result;
}

} // namespace micecam::plugin

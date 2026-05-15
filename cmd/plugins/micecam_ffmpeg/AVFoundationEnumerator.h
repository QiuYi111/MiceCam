#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace micecam::plugin {

struct EnumeratedDevice {
    std::string device_id;
    std::string display_name;
    std::string persistent_id;
    std::string vendor;
    std::string serial;
    bool has_persistent_id = false;
    int max_width = 1920;
    int max_height = 1080;
    double max_framerate = 30.0;
    bool available = true;
    std::string unavailable_reason;
};

class AVFoundationEnumerator {
public:
    AVFoundationEnumerator();
    std::vector<EnumeratedDevice> enumerate() const;
    bool isAvailable() const { return available_; }

private:
    bool available_ = false;
};

} // namespace micecam::plugin

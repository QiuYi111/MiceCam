#pragma once

namespace micecam::domain {

struct EncoderConfig {
    int bitrate_kbps = 5000;
    int keyframe_interval = 60;
    int crf = 18;
    int max_b_frames = 0;
    bool prefer_hardware = true;
};

} // namespace micecam::domain

#include "StreamStats.h"

#include <nlohmann/json.hpp>

namespace micecam::domain {

nlohmann::json StreamStats::to_json() const {
    nlohmann::json j;
    j["stream_id"] = stream_id;
    j["frames_expected"] = frames_expected;
    j["frames_actual"] = frames_actual;
    j["drop_rate"] = drop_rate;
    j["avg_encode_latency_us"] = avg_encode_latency_us;
    j["max_encode_latency_us"] = max_encode_latency_us;
    j["min_encode_latency_us"] = min_encode_latency_us;
    j["avg_frame_interval_us"] = avg_frame_interval_us;
    j["max_frame_interval_deviation_us"] = max_frame_interval_deviation_us;
    j["bytes_written"] = bytes_written;
    j["encoder_used"] = encoder_used;
    j["encoder_fallback"] = encoder_fallback;
    if (!transport.is_null()) {
        j["transport"] = transport;
    }
    return j;
}

} // namespace micecam::domain

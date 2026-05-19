#include "SessionMetadata.h"

namespace micecam::domain {

nlohmann::json SessionMetadata::to_json() const {
    nlohmann::json j;
    j["session_id"] = session_id;
    j["wall_clock_anchor_ns"] = wall_clock_anchor_ns;
    j["encoder_name"] = encoder_name;
    j["bitrate_kbps"] = bitrate_kbps;
    j["keyframe_interval"] = keyframe_interval;
    j["output_dir"] = output_dir;
    j["start_time_ns"] = start_time_ns;
    j["end_time_ns"] = end_time_ns;
    nlohmann::json configs = nlohmann::json::array();
    for (const auto& sc : stream_configs) {
        nlohmann::json c;
        c["device_id"] = sc.device_id;
        c["stream_index"] = sc.stream_index;
        c["width"] = sc.width;
        c["height"] = sc.height;
        c["framerate"] = sc.framerate;
        c["pixel_format"] = sc.pixel_format;
        configs.push_back(c);
    }
    j["stream_configs"] = configs;
    if (!plugin_source.is_null()) {
        j["plugin_source"] = plugin_source;
    }
    return j;
}

SessionMetadata SessionMetadata::from_json(const nlohmann::json& j) {
    SessionMetadata m;
    m.session_id = j.value("session_id", "");
    m.wall_clock_anchor_ns = j.value("wall_clock_anchor_ns", 0ULL);
    m.encoder_name = j.value("encoder_name", "");
    m.bitrate_kbps = j.value("bitrate_kbps", 0);
    m.keyframe_interval = j.value("keyframe_interval", 0);
    m.output_dir = j.value("output_dir", "");
    m.start_time_ns = j.value("start_time_ns", 0ULL);
    m.end_time_ns = j.value("end_time_ns", 0ULL);
    if (j.contains("stream_configs") && j["stream_configs"].is_array()) {
        for (const auto& c : j["stream_configs"]) {
            StreamConfig sc;
            sc.device_id = c.value("device_id", "");
            sc.stream_index = c.value("stream_index", 0);
            sc.width = c.value("width", 0);
            sc.height = c.value("height", 0);
            sc.framerate = c.value("framerate", 0);
            sc.pixel_format = c.value("pixel_format", "");
            m.stream_configs.push_back(sc);
        }
    }
    if (j.contains("plugin_source") && j["plugin_source"].is_object()) {
        m.plugin_source = j["plugin_source"];
    }
    return m;
}

} // namespace micecam::domain

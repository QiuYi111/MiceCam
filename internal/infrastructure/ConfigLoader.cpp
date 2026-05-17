#include "ConfigLoader.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace micecam::infrastructure {

bool ConfigLoader::load(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return true;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }

    if (j.contains("watchdog_timeout_s")) watchdog_timeout_s_ = j["watchdog_timeout_s"].get<int>();
    if (j.contains("drop_rate_yellow_pct")) drop_rate_yellow_pct_ = j["drop_rate_yellow_pct"].get<double>();
    if (j.contains("drop_rate_red_pct")) drop_rate_red_pct_ = j["drop_rate_red_pct"].get<double>();
    if (j.contains("webhook_url")) webhook_url_ = j["webhook_url"].get<std::string>();
    if (j.contains("default_bitrate_kbps")) default_bitrate_kbps_ = j["default_bitrate_kbps"].get<int>();
    if (j.contains("output_dir")) output_dir_ = j["output_dir"].get<std::string>();
    if (j.contains("log_level")) log_level_ = j["log_level"].get<std::string>();
    if (j.contains("keyframe_interval")) keyframe_interval_ = j["keyframe_interval"].get<int>();
    if (j.contains("encoder_preset")) encoder_preset_ = j["encoder_preset"].get<std::string>();
    if (j.contains("hardware_acceleration")) hardware_acceleration_ = j["hardware_acceleration"].get<bool>();
    if (j.contains("preview_quality")) preview_quality_ = j["preview_quality"].get<std::string>();
    if (j.contains("desktop_notifications")) desktop_notifications_ = j["desktop_notifications"].get<bool>();
    if (j.contains("sound_alerts")) sound_alerts_ = j["sound_alerts"].get<bool>();
    if (j.contains("verbose_diagnostics")) verbose_diagnostics_ = j["verbose_diagnostics"].get<bool>();
    if (j.contains("create_subfolder")) create_subfolder_per_session_ = j["create_subfolder"].get<bool>();
    if (j.contains("folder_name_prefix")) folder_name_prefix_ = j["folder_name_prefix"].get<std::string>();
    if (j.contains("naming_pattern")) naming_pattern_ = j["naming_pattern"].get<std::string>();
    if (j.contains("container_format")) container_format_ = j["container_format"].get<std::string>();
    if (j.contains("max_file_size_gb")) max_file_size_gb_ = j["max_file_size_gb"].get<int>();

    return true;
}

bool ConfigLoader::save(const std::string& config_path) const {
    nlohmann::json j;
    j["watchdog_timeout_s"] = watchdog_timeout_s_;
    j["drop_rate_yellow_pct"] = drop_rate_yellow_pct_;
    j["drop_rate_red_pct"] = drop_rate_red_pct_;
    j["webhook_url"] = webhook_url_;
    j["default_bitrate_kbps"] = default_bitrate_kbps_;
    j["output_dir"] = output_dir_;
    j["log_level"] = log_level_;
    j["keyframe_interval"] = keyframe_interval_;
    j["encoder_preset"] = encoder_preset_;
    j["hardware_acceleration"] = hardware_acceleration_;
    j["preview_quality"] = preview_quality_;
    j["desktop_notifications"] = desktop_notifications_;
    j["sound_alerts"] = sound_alerts_;
    j["verbose_diagnostics"] = verbose_diagnostics_;
    j["create_subfolder"] = create_subfolder_per_session_;
    j["folder_name_prefix"] = folder_name_prefix_;
    j["naming_pattern"] = naming_pattern_;
    j["container_format"] = container_format_;
    j["max_file_size_gb"] = max_file_size_gb_;

    std::ofstream file(config_path);
    if (!file.is_open()) return false;
    file << j.dump(2);
    return file.good();
}

} // namespace micecam::infrastructure

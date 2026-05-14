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

    return true;
}

} // namespace micecam::infrastructure

#pragma once

#include <string>

namespace micecam::infrastructure {

class ConfigLoader {
public:
    bool load(const std::string& config_path);

    int watchdog_timeout_s() const { return watchdog_timeout_s_; }
    double drop_rate_yellow_pct() const { return drop_rate_yellow_pct_; }
    double drop_rate_red_pct() const { return drop_rate_red_pct_; }
    std::string webhook_url() const { return webhook_url_; }
    int default_bitrate_kbps() const { return default_bitrate_kbps_; }
    std::string output_dir() const { return output_dir_; }
    std::string log_level() const { return log_level_; }

private:
    int watchdog_timeout_s_ = 3;
    double drop_rate_yellow_pct_ = 0.1;
    double drop_rate_red_pct_ = 1.0;
    std::string webhook_url_;
    int default_bitrate_kbps_ = 5000;
    std::string output_dir_;
    std::string log_level_ = "info";
};

} // namespace micecam::infrastructure

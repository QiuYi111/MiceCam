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

    void set_watchdog_timeout_s(int value) { watchdog_timeout_s_ = value; }
    void set_drop_rate_yellow_pct(double value) { drop_rate_yellow_pct_ = value; }
    void set_drop_rate_red_pct(double value) { drop_rate_red_pct_ = value; }
    void set_webhook_url(std::string value) { webhook_url_ = std::move(value); }
    void set_default_bitrate_kbps(int value) { default_bitrate_kbps_ = value; }
    void set_output_dir(std::string value) { output_dir_ = std::move(value); }
    void set_log_level(std::string value) { log_level_ = std::move(value); }
    bool save(const std::string& config_path) const;

    // New settings (002-remove-ui-mock-data)
    int keyframe_interval() const { return keyframe_interval_; }
    std::string encoder_preset() const { return encoder_preset_; }
    bool hardware_acceleration() const { return hardware_acceleration_; }
    std::string preview_quality() const { return preview_quality_; }
    bool desktop_notifications() const { return desktop_notifications_; }
    bool sound_alerts() const { return sound_alerts_; }
    bool verbose_diagnostics() const { return verbose_diagnostics_; }
    bool create_subfolder_per_session() const { return create_subfolder_per_session_; }
    std::string folder_name_prefix() const { return folder_name_prefix_; }
    std::string naming_pattern() const { return naming_pattern_; }
    std::string container_format() const { return container_format_; }
    int max_file_size_gb() const { return max_file_size_gb_; }

    void set_keyframe_interval(int v) { keyframe_interval_ = v; }
    void set_encoder_preset(std::string v) { encoder_preset_ = std::move(v); }
    void set_hardware_acceleration(bool v) { hardware_acceleration_ = v; }
    void set_preview_quality(std::string v) { preview_quality_ = std::move(v); }
    void set_desktop_notifications(bool v) { desktop_notifications_ = v; }
    void set_sound_alerts(bool v) { sound_alerts_ = v; }
    void set_verbose_diagnostics(bool v) { verbose_diagnostics_ = v; }
    void set_create_subfolder_per_session(bool v) { create_subfolder_per_session_ = v; }
    void set_folder_name_prefix(std::string v) { folder_name_prefix_ = std::move(v); }
    void set_naming_pattern(std::string v) { naming_pattern_ = std::move(v); }
    void set_container_format(std::string v) { container_format_ = std::move(v); }
    void set_max_file_size_gb(int v) { max_file_size_gb_ = v; }

private:
    int watchdog_timeout_s_ = 3;
    double drop_rate_yellow_pct_ = 0.1;
    double drop_rate_red_pct_ = 1.0;
    std::string webhook_url_;
    int default_bitrate_kbps_ = 5000;
    std::string output_dir_;
    std::string log_level_ = "info";
    int keyframe_interval_ = 120;
    std::string encoder_preset_ = "medium";
    bool hardware_acceleration_ = true;
    std::string preview_quality_ = "medium";
    bool desktop_notifications_ = true;
    bool sound_alerts_ = true;
    bool verbose_diagnostics_ = false;
    bool create_subfolder_per_session_ = true;
    std::string folder_name_prefix_ = "session_";
    std::string naming_pattern_ = "CAM_NAME_YYYYMMDD_HHMMSS";
    std::string container_format_ = "mp4";
    int max_file_size_gb_ = 8;
};

} // namespace micecam::infrastructure

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "infrastructure/ConfigLoader.h"

using namespace micecam;

namespace {

#ifdef _WIN32
constexpr const char* TEST_DIR = ".";
constexpr const char* TEST_OUTPUT = "./test_output";
#else
constexpr const char* TEST_DIR = "/tmp";
constexpr const char* TEST_OUTPUT = "/tmp/output";
#endif

std::string write_temp_json(const std::string& content) {
    std::string path = std::string(TEST_DIR) + "/micecam_test_config_" +
                       std::to_string(std::rand()) + ".json";
    std::ofstream f(path);
    f << content;
    f.close();
    return path;
}

} // namespace

TEST(ConfigLoader, LoadValidConfig) {
    std::string json = std::string(R"({
        "watchdog_timeout_s": 5,
        "drop_rate_yellow_pct": 0.5,
        "drop_rate_red_pct": 2.0,
        "webhook_url": "https://example.com/webhook",
        "default_bitrate_kbps": 8000,
        "output_dir": ")") + TEST_OUTPUT + R"(",
        "log_level": "debug"
    })";
    std::string path = write_temp_json(json);

    infrastructure::ConfigLoader loader;
    ASSERT_TRUE(loader.load(path));

    EXPECT_EQ(loader.watchdog_timeout_s(), 5);
    EXPECT_DOUBLE_EQ(loader.drop_rate_yellow_pct(), 0.5);
    EXPECT_DOUBLE_EQ(loader.drop_rate_red_pct(), 2.0);
    EXPECT_EQ(loader.webhook_url(), "https://example.com/webhook");
    EXPECT_EQ(loader.default_bitrate_kbps(), 8000);
    EXPECT_EQ(loader.output_dir(), TEST_OUTPUT);
    EXPECT_EQ(loader.log_level(), "debug");

    std::remove(path.c_str());
}

TEST(ConfigLoader, MissingFileReturnsDefaults) {
    infrastructure::ConfigLoader loader;
    ASSERT_TRUE(loader.load("/nonexistent/path/config.json"));

    EXPECT_EQ(loader.watchdog_timeout_s(), 3);
    EXPECT_DOUBLE_EQ(loader.drop_rate_yellow_pct(), 0.1);
    EXPECT_DOUBLE_EQ(loader.drop_rate_red_pct(), 1.0);
    EXPECT_TRUE(loader.webhook_url().empty());
    EXPECT_EQ(loader.default_bitrate_kbps(), 5000);
    EXPECT_TRUE(loader.output_dir().empty());
    EXPECT_EQ(loader.log_level(), "info");
}

TEST(ConfigLoader, PartialConfigMergesWithDefaults) {
    std::string json = R"({
        "watchdog_timeout_s": 10,
        "output_dir": "/custom/output"
    })";
    std::string path = write_temp_json(json);

    infrastructure::ConfigLoader loader;
    ASSERT_TRUE(loader.load(path));

    EXPECT_EQ(loader.watchdog_timeout_s(), 10);
    EXPECT_FALSE(loader.output_dir().empty());
    EXPECT_DOUBLE_EQ(loader.drop_rate_yellow_pct(), 0.1);
    EXPECT_EQ(loader.default_bitrate_kbps(), 5000);
    EXPECT_TRUE(loader.webhook_url().empty());
    EXPECT_EQ(loader.log_level(), "info");

    std::remove(path.c_str());
}

TEST(ConfigLoader, InvalidJsonReturnsFalse) {
    std::string json = "{ not valid json }";
    std::string path = write_temp_json(json);

    infrastructure::ConfigLoader loader;
    EXPECT_FALSE(loader.load(path));

    std::remove(path.c_str());
}

TEST(ConfigLoader, SavePersistsNewSettingsProperties) {
    const std::string path = std::string(TEST_DIR) + "/micecam_ui_new_settings_test.json";
    std::remove(path.c_str());

    micecam::infrastructure::ConfigLoader config;
    config.set_keyframe_interval(60);
    config.set_encoder_preset("ultrafast");
    config.set_hardware_acceleration(false);
    config.set_preview_quality("low");
    config.set_desktop_notifications(false);
    config.set_sound_alerts(false);
    config.set_verbose_diagnostics(true);
    config.set_create_subfolder_per_session(false);
    config.set_folder_name_prefix("rec_");
    config.set_naming_pattern("INDEX");
    config.set_container_format("mkv");
    config.set_max_file_size_gb(16);

    ASSERT_TRUE(config.save(path));

    micecam::infrastructure::ConfigLoader loaded;
    ASSERT_TRUE(loaded.load(path));
    EXPECT_EQ(loaded.keyframe_interval(), 60);
    EXPECT_EQ(loaded.encoder_preset(), "ultrafast");
    EXPECT_EQ(loaded.hardware_acceleration(), false);
    EXPECT_EQ(loaded.preview_quality(), "low");
    EXPECT_EQ(loaded.desktop_notifications(), false);
    EXPECT_EQ(loaded.sound_alerts(), false);
    EXPECT_EQ(loaded.verbose_diagnostics(), true);
    EXPECT_EQ(loaded.create_subfolder_per_session(), false);
    EXPECT_EQ(loaded.folder_name_prefix(), "rec_");
    EXPECT_EQ(loaded.naming_pattern(), "INDEX");
    EXPECT_EQ(loaded.container_format(), "mkv");
    EXPECT_EQ(loaded.max_file_size_gb(), 16);
}

TEST(ConfigLoader, SavePersistsUiEditableSettings) {
    const std::string path = "/tmp/micecam_ui_settings_test.json";
    std::remove(path.c_str());

    micecam::infrastructure::ConfigLoader config;
    config.set_watchdog_timeout_s(7);
    config.set_drop_rate_yellow_pct(0.2);
    config.set_drop_rate_red_pct(1.5);
    config.set_webhook_url("https://example.invalid/hook");
    config.set_default_bitrate_kbps(8000);
    config.set_output_dir(TEST_OUTPUT);
    config.set_log_level("debug");

    ASSERT_TRUE(config.save(path));

    micecam::infrastructure::ConfigLoader loaded;
    ASSERT_TRUE(loaded.load(path));
    EXPECT_EQ(loaded.watchdog_timeout_s(), 7);
    EXPECT_DOUBLE_EQ(loaded.drop_rate_yellow_pct(), 0.2);
    EXPECT_DOUBLE_EQ(loaded.drop_rate_red_pct(), 1.5);
    EXPECT_EQ(loaded.webhook_url(), "https://example.invalid/hook");
    EXPECT_EQ(loaded.default_bitrate_kbps(), 8000);
    EXPECT_EQ(loaded.output_dir(), TEST_OUTPUT);
    EXPECT_EQ(loaded.log_level(), "debug");
}

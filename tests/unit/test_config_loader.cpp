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

TEST(ConfigLoader, SavePersistsUiEditableSettings) {
    const std::string path = "/tmp/micecam_ui_settings_test.json";
    std::remove(path.c_str());

    micecam::infrastructure::ConfigLoader config;
    config.set_watchdog_timeout_s(7);
    config.set_drop_rate_yellow_pct(0.2);
    config.set_drop_rate_red_pct(1.5);
    config.set_webhook_url("https://example.invalid/hook");
    config.set_default_bitrate_kbps(8000);
    config.set_output_dir("/tmp/micecam-output");
    config.set_log_level("debug");

    ASSERT_TRUE(config.save(path));

    micecam::infrastructure::ConfigLoader loaded;
    ASSERT_TRUE(loaded.load(path));
    EXPECT_EQ(loaded.watchdog_timeout_s(), 7);
    EXPECT_DOUBLE_EQ(loaded.drop_rate_yellow_pct(), 0.2);
    EXPECT_DOUBLE_EQ(loaded.drop_rate_red_pct(), 1.5);
    EXPECT_EQ(loaded.webhook_url(), "https://example.invalid/hook");
    EXPECT_EQ(loaded.default_bitrate_kbps(), 8000);
    EXPECT_EQ(loaded.output_dir(), "/tmp/micecam-output");
    EXPECT_EQ(loaded.log_level(), "debug");
}

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>
#include <string>

#include "infrastructure/LinkedPluginConfig.h"

namespace fs = std::filesystem;
using namespace micecam::infrastructure;

class LinkedPluginConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "micecam_test_linked_config";
        fs::create_directories(test_dir_);
        config_path_ = (test_dir_ / "linked_plugins.json").string();
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
    std::string config_path_;
};

TEST_F(LinkedPluginConfigTest, new_config_loads_empty_list) {
    LinkedPluginConfig config(config_path_);
    ASSERT_TRUE(config.load());
    EXPECT_TRUE(config.paths().empty());
}

TEST_F(LinkedPluginConfigTest, missing_config_file_returns_empty_no_crash) {
    LinkedPluginConfig config("/nonexistent/path/to/linked_plugins.json");
    ASSERT_TRUE(config.load());
    EXPECT_TRUE(config.paths().empty());
}

TEST_F(LinkedPluginConfigTest, add_path_increases_count) {
    LinkedPluginConfig config(config_path_);
    config.add("/test/path/one");
    EXPECT_EQ(config.paths().size(), 1u);
    EXPECT_EQ(config.paths()[0], "/test/path/one");
}

TEST_F(LinkedPluginConfigTest, add_duplicate_path_ignored) {
    LinkedPluginConfig config(config_path_);
    config.add("/test/path/one");
    config.add("/test/path/one");
    EXPECT_EQ(config.paths().size(), 1u);
}

TEST_F(LinkedPluginConfigTest, remove_path_decreases_count) {
    LinkedPluginConfig config(config_path_);
    config.add("/test/path/one");
    config.add("/test/path/two");
    config.remove("/test/path/one");
    EXPECT_EQ(config.paths().size(), 1u);
    EXPECT_EQ(config.paths()[0], "/test/path/two");
}

TEST_F(LinkedPluginConfigTest, remove_nonexistent_path_is_noop) {
    LinkedPluginConfig config(config_path_);
    config.add("/test/path/one");
    config.remove("/test/path/nonexistent");
    EXPECT_EQ(config.paths().size(), 1u);
}

TEST_F(LinkedPluginConfigTest, save_and_load_round_trip) {
    {
        LinkedPluginConfig config(config_path_);
        config.add("/plugins/alpha");
        config.add("/plugins/beta");
        ASSERT_TRUE(config.save());
    }

    {
        LinkedPluginConfig config(config_path_);
        ASSERT_TRUE(config.load());
        auto paths = config.paths();
        ASSERT_EQ(paths.size(), 2u);
        EXPECT_EQ(paths[0], "/plugins/alpha");
        EXPECT_EQ(paths[1], "/plugins/beta");
    }
}

TEST_F(LinkedPluginConfigTest, empty_config_saves_and_loads) {
    {
        LinkedPluginConfig config(config_path_);
        ASSERT_TRUE(config.save());
    }

    {
        LinkedPluginConfig config(config_path_);
        ASSERT_TRUE(config.load());
        EXPECT_TRUE(config.paths().empty());
    }
}

TEST_F(LinkedPluginConfigTest, add_remove_save_load_full_cycle) {
    {
        LinkedPluginConfig config(config_path_);
        config.add("/a");
        config.add("/b");
        config.add("/c");
        ASSERT_TRUE(config.save());
    }

    {
        LinkedPluginConfig config(config_path_);
        ASSERT_TRUE(config.load());
        EXPECT_EQ(config.paths().size(), 3u);
        config.remove("/b");
        ASSERT_TRUE(config.save());
    }

    {
        LinkedPluginConfig config(config_path_);
        ASSERT_TRUE(config.load());
        auto paths = config.paths();
        ASSERT_EQ(paths.size(), 2u);
        EXPECT_EQ(paths[0], "/a");
        EXPECT_EQ(paths[1], "/c");
    }
}

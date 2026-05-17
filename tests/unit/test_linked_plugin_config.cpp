#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>
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
    EXPECT_TRUE(config.entries().empty());
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
    EXPECT_EQ(config.entries().size(), 1u);
    EXPECT_TRUE(config.entries()[0].enabled);
    EXPECT_FALSE(config.entries()[0].added_at.empty());
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
        auto& entries = config.entries();
        EXPECT_EQ(entries.size(), 2u);
        EXPECT_TRUE(entries[0].enabled);
        EXPECT_FALSE(entries[0].added_at.empty());
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

TEST_F(LinkedPluginConfigTest, backward_compat_loads_old_string_array) {
    {
        std::ofstream file(config_path_);
        nlohmann::json j;
        j["linked_plugins"] = nlohmann::json::array({"/old/plugin1", "/old/plugin2"});
        file << j.dump(2);
    }

    LinkedPluginConfig config(config_path_);
    ASSERT_TRUE(config.load());
    auto paths = config.paths();
    ASSERT_EQ(paths.size(), 2u);
    EXPECT_EQ(paths[0], "/old/plugin1");
    EXPECT_EQ(paths[1], "/old/plugin2");
    auto& entries = config.entries();
    EXPECT_TRUE(entries[0].enabled);
    EXPECT_TRUE(entries[0].added_at.empty());
}

TEST_F(LinkedPluginConfigTest, set_enabled_disables_entry) {
    LinkedPluginConfig config(config_path_);
    config.add("/plugin/alpha");
    config.add("/plugin/beta");

    EXPECT_TRUE(config.set_enabled("/plugin/alpha", false));
    auto paths = config.paths();
    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], "/plugin/beta");
    EXPECT_EQ(config.entries().size(), 2u);
}

TEST_F(LinkedPluginConfigTest, set_enabled_nonexistent_returns_false) {
    LinkedPluginConfig config(config_path_);
    EXPECT_FALSE(config.set_enabled("/nonexistent", true));
}

TEST_F(LinkedPluginConfigTest, entries_accessor_returns_all) {
    LinkedPluginConfig config(config_path_);
    config.add("/p1");
    config.add("/p2");
    config.set_enabled("/p1", false);

    auto& entries = config.entries();
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_FALSE(entries[0].enabled);
    EXPECT_TRUE(entries[1].enabled);
}

TEST_F(LinkedPluginConfigTest, save_outputs_object_format) {
    {
        LinkedPluginConfig config(config_path_);
        config.add("/test/plugin");
        ASSERT_TRUE(config.save());
    }

    std::ifstream file(config_path_);
    nlohmann::json j;
    file >> j;
    ASSERT_TRUE(j.contains("linked_plugins"));
    ASSERT_EQ(j["linked_plugins"].size(), 1u);
    EXPECT_TRUE(j["linked_plugins"][0].is_object());
    EXPECT_EQ(j["linked_plugins"][0]["path"].get<std::string>(), "/test/plugin");
    EXPECT_TRUE(j["linked_plugins"][0]["enabled"].get<bool>());
    EXPECT_TRUE(j["linked_plugins"][0].contains("added_at"));
}

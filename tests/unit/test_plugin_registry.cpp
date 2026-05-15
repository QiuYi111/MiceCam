#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "infrastructure/PluginRegistryService.h"

namespace fs = std::filesystem;
using namespace micecam::infrastructure;

class PluginRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_root_ = fs::temp_directory_path() / "micecam_test_registry";
        fs::create_directories(test_root_);
        config_dir_ = (test_root_ / "config").string();
        bundled_dir_ = (test_root_ / "bundled").string();
        fs::create_directories(config_dir_);
        fs::create_directories(bundled_dir_);

        createValidBundledPlugin();
    }

    void TearDown() override {
        fs::remove_all(test_root_);
    }

    void createValidBundledPlugin() {
        auto dir = fs::path(bundled_dir_) / "micecam.valid";
        fs::create_directories(dir);
        std::ofstream f(dir / "plugin.json");
        f << R"({
            "id": "micecam.valid",
            "name": "Valid Test Plugin",
            "version": "1.0.0",
            "plugin_api_version": 1,
            "min_micecam_version": "2.0.0",
            "platforms": {
                "darwin": {"entrypoint": "bin/test", "arch": "universal"}
            },
            "supported_process_models": ["SINGLETON"],
            "preferred_process_model": "SINGLETON"
        })";
        f.close();
    }

    void createInvalidBundledPlugin() {
        auto dir = fs::path(bundled_dir_) / "micecam.invalid";
        fs::create_directories(dir);
        std::ofstream f(dir / "plugin.json");
        f << R"({
            "name": "Invalid Plugin"
        })";
        f.close();
    }

    std::string createValidLinkedDir(const std::string& subdir) {
        auto dir = test_root_ / subdir;
        fs::create_directories(dir);
        std::ofstream f(dir / "plugin.json");
        f << R"({
            "id": "micecam.linked",
            "name": "Linked Test Plugin",
            "version": "1.0.0",
            "plugin_api_version": 1,
            "min_micecam_version": "2.0.0",
            "platforms": {
                "darwin": {"entrypoint": "bin/test", "arch": "universal"}
            },
            "supported_process_models": ["PER_DEVICE"],
            "preferred_process_model": "PER_DEVICE"
        })";
        f.close();
        return dir.string();
    }

    void createLinkedDirWithMissingJson(const std::string& subdir) {
        auto dir = test_root_ / subdir;
        fs::create_directories(dir);
    }

    void createLinkedDirWithInvalidJson(const std::string& subdir) {
        auto dir = test_root_ / subdir;
        fs::create_directories(dir);
        std::ofstream f(dir / "plugin.json");
        f << R"({"not-a-valid-manifest": true})";
        f.close();
    }

    fs::path test_root_;
    std::string config_dir_;
    std::string bundled_dir_;
};

TEST_F(PluginRegistryTest, bundled_plugin_with_valid_manifest_discovered) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    auto plugins = service.getPlugins();
    ASSERT_EQ(plugins.size(), 1u);
    EXPECT_EQ(plugins[0].id, "micecam.valid");
    EXPECT_EQ(plugins[0].name, "Valid Test Plugin");
    EXPECT_EQ(plugins[0].source_type, micecam::domain::PluginSourceType::BUNDLED);
    EXPECT_TRUE(plugins[0].enabled);
}

TEST_F(PluginRegistryTest, bundled_plugin_with_invalid_manifest_registered_with_diagnostic) {
    createInvalidBundledPlugin();

    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    auto& diags = service.getDiagnostics();
    ASSERT_GE(diags.size(), 1u);
    EXPECT_EQ(diags[0].error_code, "MANIFEST_PARSE_ERROR");
}

TEST_F(PluginRegistryTest, missing_bundled_directory_handled_gracefully) {
    PluginRegistryService service("/nonexistent/bundled/plugins", config_dir_);
    ASSERT_TRUE(service.initialize());

    auto plugins = service.getPlugins();
    EXPECT_TRUE(plugins.empty());

    auto& diags = service.getDiagnostics();
    EXPECT_TRUE(diags.empty());
}

TEST_F(PluginRegistryTest, add_linked_directory_with_valid_manifest_accepted) {
    auto linked_dir = createValidLinkedDir("linked_valid");

    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    EXPECT_TRUE(service.addLinkedDirectory(linked_dir));
    EXPECT_TRUE(service.isPendingRestart());
}

TEST_F(PluginRegistryTest, add_linked_directory_with_missing_json_rejected) {
    createLinkedDirWithMissingJson("linked_nojson");

    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    auto linked_path = (test_root_ / "linked_nojson").string();
    EXPECT_FALSE(service.addLinkedDirectory(linked_path));

    auto& diags = service.getDiagnostics();
    ASSERT_GE(diags.size(), 1u);
    EXPECT_NE(diags.back().message.find("plugin.json not found"), std::string::npos);
}

TEST_F(PluginRegistryTest, add_linked_directory_with_invalid_schema_rejected) {
    createLinkedDirWithInvalidJson("linked_badschema");

    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    auto linked_path = (test_root_ / "linked_badschema").string();
    EXPECT_FALSE(service.addLinkedDirectory(linked_path));

    auto& diags = service.getDiagnostics();
    ASSERT_GE(diags.size(), 1u);
    EXPECT_EQ(diags.back().error_code, "MANIFEST_PARSE_ERROR");
}

TEST_F(PluginRegistryTest, remove_linked_directory_removes_from_config) {
    auto linked_dir = createValidLinkedDir("linked_removable");

    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    EXPECT_TRUE(service.addLinkedDirectory(linked_dir));
    EXPECT_TRUE(service.removeLinkedDirectory(linked_dir));
    EXPECT_TRUE(service.isPendingRestart());
}

TEST_F(PluginRegistryTest, enable_disable_toggle_pending_restart_flag) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    EXPECT_FALSE(service.isPendingRestart());

    service.disablePlugin("micecam.valid");
    EXPECT_TRUE(service.isPendingRestart());
}

TEST_F(PluginRegistryTest, get_sources_returns_grouped_sources) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    auto sources = service.getSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "micecam.valid");
    EXPECT_EQ(sources[0].source_name, "Valid Test Plugin");
    EXPECT_EQ(sources[0].source_type, micecam::domain::PluginSourceType::BUNDLED);
    EXPECT_TRUE(sources[0].enabled);
    EXPECT_EQ(sources[0].diagnostics_state, micecam::domain::PluginDiagnosticsState::OK);
}

TEST_F(PluginRegistryTest, get_plugins_returns_only_enabled) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    EXPECT_EQ(service.getPlugins().size(), 1u);

    service.disablePlugin("micecam.valid");
    EXPECT_EQ(service.getPlugins().size(), 0u);
}

TEST_F(PluginRegistryTest, discover_all_returns_enabled_plugins) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    auto all = service.discoverAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_TRUE(all[0].enabled);
}

TEST_F(PluginRegistryTest, enable_disabled_plugin) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    service.disablePlugin("micecam.valid");
    EXPECT_EQ(service.getPlugins().size(), 0u);

    service.enablePlugin("micecam.valid");
    EXPECT_EQ(service.getPlugins().size(), 1u);
    EXPECT_TRUE(service.isPendingRestart());
}

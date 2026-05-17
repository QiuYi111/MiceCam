#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "infrastructure/PluginRegistryService.h"

namespace fs = std::filesystem;
using namespace micecam::infrastructure;
using namespace std::chrono_literals;

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

class PluginCrashRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_root_ = fs::temp_directory_path() / "micecam_test_crash_recovery";
        fs::create_directories(test_root_);
        config_dir_ = (test_root_ / "config").string();
        bundled_dir_ = (test_root_ / "bundled").string();
        fs::create_directories(config_dir_);
        fs::create_directories(bundled_dir_);

        auto dir = fs::path(bundled_dir_) / "micecam.test_plugin";
        fs::create_directories(dir);
        std::ofstream f(dir / "plugin.json");
        f << R"({
            "id": "micecam.test_plugin",
            "name": "Test Plugin",
            "version": "1.0.0",
            "plugin_api_version": 2,
            "min_micecam_version": "2.0.0",
            "platforms": {
                "darwin": {"entrypoint": "bin/test", "arch": "universal"}
            },
            "supported_process_models": ["SINGLETON"],
            "preferred_process_model": "SINGLETON"
        })";
        f.close();
    }

    void TearDown() override {
        fs::remove_all(test_root_);
    }

    fs::path test_root_;
    std::string config_dir_;
    std::string bundled_dir_;
};

TEST_F(PluginCrashRecoveryTest, CrashDetectionTriggersHandlePluginCrash) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    bool alert_called = false;
    std::string alert_plugin_id;
    service.set_crash_alert_callback([&](const std::string& pid) {
        alert_called = true;
        alert_plugin_id = pid;
    });

    service.register_stream("micecam.test_plugin", "stream_a");
    service.detect_channel_failure("micecam.test_plugin");

    EXPECT_TRUE(alert_called);
    EXPECT_EQ(alert_plugin_id, "micecam.test_plugin");
}

TEST_F(PluginCrashRecoveryTest, ShmCleanupOnCrash) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    std::vector<std::string> unlinked;
    service.set_shm_unlink_fn([&](const std::string& name) -> int {
        unlinked.push_back(name);
        return 0;
    });

    service.register_shm("micecam.test_plugin", "/micecam_ring_stream_a");
    service.register_shm("micecam.test_plugin", "/micecam_ring_stream_b");

    auto result = service.handle_plugin_crash("micecam.test_plugin");

    ASSERT_EQ(result.cleaned_shm_names.size(), 2u);
    EXPECT_EQ(unlinked.size(), 2u);
    EXPECT_NE(std::find(unlinked.begin(), unlinked.end(), "/micecam_ring_stream_a"),
              unlinked.end());
    EXPECT_NE(std::find(unlinked.begin(), unlinked.end(), "/micecam_ring_stream_b"),
              unlinked.end());

    EXPECT_TRUE(service.get_shm_names_for_plugin("micecam.test_plugin").empty());
}

TEST_F(PluginCrashRecoveryTest, PerPluginIsolationOnCrash) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    service.register_stream("micecam.test_plugin", "stream_a");
    service.register_stream("other_plugin", "stream_b");

    service.register_shm("micecam.test_plugin", "/micecam_ring_stream_a");
    service.register_shm("other_plugin", "/micecam_ring_stream_b");

    std::vector<std::string> unlinked;
    service.set_shm_unlink_fn([&](const std::string& name) -> int {
        unlinked.push_back(name);
        return 0;
    });

    auto result = service.handle_plugin_crash("micecam.test_plugin");

    ASSERT_EQ(result.finalized_streams.size(), 1u);
    EXPECT_EQ(result.finalized_streams[0], "stream_a");
    EXPECT_EQ(unlinked.size(), 1u);
    EXPECT_EQ(unlinked[0], "/micecam_ring_stream_a");

    auto other_streams = service.get_streams_for_plugin("other_plugin");
    ASSERT_EQ(other_streams.size(), 1u);
    EXPECT_EQ(other_streams[0], "stream_b");

    auto other_shm = service.get_shm_names_for_plugin("other_plugin");
    ASSERT_EQ(other_shm.size(), 1u);
    EXPECT_EQ(other_shm[0], "/micecam_ring_stream_b");
}

TEST_F(PluginCrashRecoveryTest, RestartSucceedsAfterRetry) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    int restart_attempts = 0;
    service.set_restart_fn([&](const std::string&) -> bool {
        restart_attempts++;
        return restart_attempts >= 2;
    });

    service.register_stream("micecam.test_plugin", "stream_a");

    auto result = service.handle_plugin_crash("micecam.test_plugin");

    EXPECT_TRUE(result.restart_succeeded);
    EXPECT_GE(restart_attempts, 2);
}

TEST_F(PluginCrashRecoveryTest, RestartFailsAfterMaxRetries) {
    PluginRegistryService service(bundled_dir_, config_dir_);
    ASSERT_TRUE(service.initialize());

    service.set_restart_fn([](const std::string&) -> bool { return false; });

    service.register_stream("micecam.test_plugin", "stream_a");

    auto result = service.handle_plugin_crash("micecam.test_plugin");

    EXPECT_FALSE(result.restart_succeeded);
    EXPECT_TRUE(service.get_streams_for_plugin("micecam.test_plugin").empty());
}

class StreamMonitorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_root_ = fs::temp_directory_path() / "micecam_test_monitor_integration";
        fs::create_directories(test_root_);
        config_dir_ = (test_root_ / "config").string();
        bundled_dir_ = (test_root_ / "bundled").string();
        fs::create_directories(config_dir_);
        fs::create_directories(bundled_dir_);

        auto dir = fs::path(bundled_dir_) / "micecam.mon_plugin";
        fs::create_directories(dir);
        std::ofstream f(dir / "plugin.json");
        f << R"({
            "id": "micecam.mon_plugin",
            "name": "Monitor Test Plugin",
            "version": "1.0.0",
            "plugin_api_version": 2,
            "min_micecam_version": "2.0.0",
            "platforms": {
                "darwin": {"entrypoint": "bin/test", "arch": "universal"}
            },
            "supported_process_models": ["SINGLETON"],
            "preferred_process_model": "SINGLETON"
        })";
        f.close();
    }

    void TearDown() override {
        fs::remove_all(test_root_);
    }

    fs::path test_root_;
    std::string config_dir_;
    std::string bundled_dir_;
};

TEST_F(StreamMonitorIntegrationTest, AllStalledTriggersHandlePluginCrash) {
    PluginRegistryService service(bundled_dir_, config_dir_, 1000);
    ASSERT_TRUE(service.initialize());

    bool crash_alert_fired = false;
    std::string crashed_plugin;
    service.set_crash_alert_callback([&](const std::string& pid) {
        crash_alert_fired = true;
        crashed_plugin = pid;
    });

    std::vector<std::string> unlinked;
    service.set_shm_unlink_fn([&](const std::string& name) -> int {
        unlinked.push_back(name);
        return 0;
    });

    service.set_restart_fn([](const std::string&) -> bool { return false; });

    service.register_stream("micecam.mon_plugin", "stream_x");
    service.register_shm("micecam.mon_plugin", "/micecam_ring_stream_x");

    std::this_thread::sleep_for(2500ms);

    EXPECT_TRUE(crash_alert_fired);
    EXPECT_EQ(crashed_plugin, "micecam.mon_plugin");
    EXPECT_EQ(unlinked.size(), 1u);
    EXPECT_EQ(unlinked[0], "/micecam_ring_stream_x");

    auto streams = service.get_streams_for_plugin("micecam.mon_plugin");
    EXPECT_TRUE(streams.empty());
}

TEST_F(StreamMonitorIntegrationTest, StallCallbackInvokesNotifyStallFn) {
    PluginRegistryService service(bundled_dir_, config_dir_, 1000);
    ASSERT_TRUE(service.initialize());

    std::mutex notify_mutex;
    std::vector<std::tuple<std::string, std::string, uint64_t>> stall_notifications;

    service.set_notify_stall_fn([&](const std::string& stream_id,
                                     const std::string& plugin_id,
                                     uint64_t stall_duration_ms) -> StallNotifyResult {
        std::lock_guard<std::mutex> lock(notify_mutex);
        stall_notifications.push_back({stream_id, plugin_id, stall_duration_ms});
        return {true, true};
    });

    service.register_stream("micecam.mon_plugin", "stream_y");

    std::this_thread::sleep_for(2500ms);

    std::lock_guard<std::mutex> lock(notify_mutex);
    ASSERT_FALSE(stall_notifications.empty());
    auto& [sid, pid, dur] = stall_notifications[0];
    EXPECT_EQ(sid, "stream_y");
    EXPECT_EQ(pid, "micecam.mon_plugin");
    EXPECT_GE(dur, 1000u);
}

TEST_F(StreamMonitorIntegrationTest, UnrecoverableStallFinalizesStream) {
    PluginRegistryService service(bundled_dir_, config_dir_, 1000);
    ASSERT_TRUE(service.initialize());

    service.set_notify_stall_fn([](const std::string&, const std::string&,
                                    uint64_t) -> StallNotifyResult {
        return {true, false};
    });

    bool alert_fired = false;
    service.set_crash_alert_callback([&](const std::string&) {
        alert_fired = true;
    });

    service.register_stream("micecam.mon_plugin", "stream_z");

    std::this_thread::sleep_for(2500ms);

    EXPECT_TRUE(alert_fired);
    auto streams = service.get_streams_for_plugin("micecam.mon_plugin");
    EXPECT_TRUE(streams.empty());
}

TEST_F(StreamMonitorIntegrationTest, NotifyStallFailureTriggersCrashRecovery) {
    PluginRegistryService service(bundled_dir_, config_dir_, 1000);
    ASSERT_TRUE(service.initialize());

    service.set_notify_stall_fn([](const std::string&, const std::string&,
                                    uint64_t) -> StallNotifyResult {
        return {false, false};
    });

    bool crash_alert_fired = false;
    service.set_crash_alert_callback([&](const std::string&) {
        crash_alert_fired = true;
    });

    service.register_stream("micecam.mon_plugin", "stream_w");

    std::this_thread::sleep_for(2500ms);

    EXPECT_TRUE(crash_alert_fired);
}

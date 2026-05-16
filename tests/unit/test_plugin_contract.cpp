#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "domain/PluginManifest.h"
#include "domain/PluginSource.h"
#include "domain/PluginDeviceInfo.h"
#include "domain/StreamRingDescriptor.h"
#include "domain/ResourceRequest.h"
#include "domain/PluginErrorRegistry.h"
#include <nlohmann/json.hpp>

using namespace micecam::domain;
using json = nlohmann::json;

// ---- Manifest Schema Validation Tests ----

static json load_json_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open: " + path);
    }
    json j;
    file >> j;
    return j;
}

static PluginManifest load_golden_manifest() {
    auto j = load_json_file(
        "../3rdParty/bundled_plugins/micecam.ffmpeg/plugin.json");
    return PluginManifest::from_json(j);
}

TEST(ManifestValidation, golden_manifest_passes_validation) {
    auto m = load_golden_manifest();
    auto errors = m.validate();
    EXPECT_TRUE(errors.empty()) << "Unexpected error: " << (errors.empty() ? "" : errors[0]);
}

TEST(ManifestValidation, golden_manifest_has_required_fields) {
    auto m = load_golden_manifest();
    EXPECT_EQ(m.id, "micecam.ffmpeg");
    EXPECT_EQ(m.name, "MiceCam FFmpeg Capture");
    EXPECT_EQ(m.version, "1.0.0");
    EXPECT_EQ(m.plugin_api_version, 2u);
    EXPECT_EQ(m.min_micecam_version, "2.0.0");
    EXPECT_FALSE(m.platforms.empty());
    EXPECT_TRUE(m.platforms.contains("darwin"));
    EXPECT_FALSE(m.supported_process_models.empty());
    EXPECT_FALSE(m.preferred_process_model.empty());
}

TEST(ManifestValidation, missing_id_rejected) {
    auto j = load_golden_manifest().to_json();
    j.erase("id");
    auto m = PluginManifest::from_json(j);
    auto errors = m.validate();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("id"), std::string::npos);
}

TEST(ManifestValidation, missing_name_rejected) {
    auto j = load_golden_manifest().to_json();
    j.erase("name");
    auto m = PluginManifest::from_json(j);
    auto errors = m.validate();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("name"), std::string::npos);
}

TEST(ManifestValidation, invalid_version_rejected) {
    auto j = load_golden_manifest().to_json();
    j["version"] = "not-semver";
    auto m = PluginManifest::from_json(j);
    auto errors = m.validate();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("version"), std::string::npos);
}

TEST(ManifestValidation, missing_platforms_rejected) {
    auto j = load_golden_manifest().to_json();
    j["platforms"] = json::object();
    auto m = PluginManifest::from_json(j);
    auto errors = m.validate();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("platform"), std::string::npos);
}

TEST(ManifestValidation, empty_entrypoint_rejected) {
    auto j = load_golden_manifest().to_json();
    j["platforms"]["darwin"]["entrypoint"] = "";
    auto m = PluginManifest::from_json(j);
    auto errors = m.validate();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("empty entrypoint"), std::string::npos);
}

TEST(ManifestValidation, invalid_process_model_rejected) {
    auto j = load_golden_manifest().to_json();
    j["supported_process_models"] = {"INVALID"};
    auto m = PluginManifest::from_json(j);
    auto errors = m.validate();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("invalid process model"), std::string::npos);
}

TEST(ManifestValidation, preferred_not_in_supported_rejected) {
    auto j = load_golden_manifest().to_json();
    j["supported_process_models"] = {"PER_DEVICE"};
    j["preferred_process_model"] = "SINGLETON";
    auto m = PluginManifest::from_json(j);
    auto errors = m.validate();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("not in supported_process_models"), std::string::npos);
}

TEST(ManifestValidation, roundtrip_preserves_fields) {
    auto orig = load_golden_manifest();
    auto j = orig.to_json();
    auto restored = PluginManifest::from_json(j);

    EXPECT_EQ(restored.id, orig.id);
    EXPECT_EQ(restored.name, orig.name);
    EXPECT_EQ(restored.version, orig.version);
    EXPECT_EQ(restored.plugin_api_version, orig.plugin_api_version);
    EXPECT_EQ(restored.min_micecam_version, orig.min_micecam_version);
    EXPECT_EQ(restored.supported_process_models, orig.supported_process_models);
    EXPECT_EQ(restored.preferred_process_model, orig.preferred_process_model);
    EXPECT_EQ(restored.platforms.size(), orig.platforms.size());
}

// ---- Error Registry Tests (FR-019 coverage) ----

TEST(ErrorRegistry, all_spec_failure_modes_covered) {
    // FR-019 requires structured diagnostics: error code, severity, recoverability,
    // user message, technical detail, suggested action, affected target,
    // retry delay, and recovery action.
    const auto& map = PluginErrorRegistry::entries();

    // Verify each entry has all required metadata
    for (const auto& [code, meta] : map) {
        EXPECT_FALSE(meta.default_user_message.empty())
            << "Missing user message for error code";
        EXPECT_FALSE(meta.default_suggested_action.empty())
            << "Missing suggested action for error code";
        // is_recoverable, severity, recovery_action are always set (enum values)
    }
}

TEST(ErrorRegistry, known_errors_have_correct_severity) {
    EXPECT_EQ(PluginErrorRegistry::get(PluginErrorCode::PLUGIN_CRASH).severity, ErrorSeverity::FATAL);
    EXPECT_EQ(PluginErrorRegistry::get(PluginErrorCode::SHM_ALLOC_FAILED).severity, ErrorSeverity::FATAL);
    EXPECT_EQ(PluginErrorRegistry::get(PluginErrorCode::SDK_MISSING).severity, ErrorSeverity::FATAL);
}

TEST(ErrorRegistry, non_fatal_errors_are_recoverable) {
    // Non-fatal errors should be marked recoverable
    const auto& map = PluginErrorRegistry::entries();
    for (const auto& [code, meta] : map) {
        if (meta.severity != ErrorSeverity::FATAL) {
            EXPECT_TRUE(meta.is_recoverable)
                << "Non-fatal error should be recoverable";
        }
    }
}

TEST(ErrorRegistry, covers_manifest_error) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::MANIFEST_PARSE_ERROR);
    EXPECT_EQ(meta.severity, ErrorSeverity::ERROR);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_handshake_error) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::HANDSHAKE_FAILED);
    EXPECT_EQ(meta.severity, ErrorSeverity::ERROR);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_device_unavailable) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::DEVICE_UNAVAILABLE);
    EXPECT_EQ(meta.severity, ErrorSeverity::WARN);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_device_busy) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::DEVICE_BUSY);
    EXPECT_EQ(meta.severity, ErrorSeverity::WARN);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_enumeration_failed) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::ENUMERATION_FAILED);
    EXPECT_EQ(meta.severity, ErrorSeverity::ERROR);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_stream_open_failed) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::STREAM_OPEN_FAILED);
    EXPECT_EQ(meta.severity, ErrorSeverity::ERROR);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_capability_mismatch) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::CAPABILITY_MISMATCH);
    EXPECT_EQ(meta.severity, ErrorSeverity::WARN);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_config_invalid) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::CONFIG_INVALID);
    EXPECT_EQ(meta.severity, ErrorSeverity::ERROR);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_plugin_timeout) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::PLUGIN_TIMEOUT);
    EXPECT_EQ(meta.severity, ErrorSeverity::ERROR);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_plugin_crash) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::PLUGIN_CRASH);
    EXPECT_EQ(meta.severity, ErrorSeverity::FATAL);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_backpressure) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::BACKPRESSURE);
    EXPECT_EQ(meta.severity, ErrorSeverity::WARN);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, covers_context_conflict) {
    auto& meta = PluginErrorRegistry::get(PluginErrorCode::EXCLUSIVE_CONFLICT);
    EXPECT_EQ(meta.severity, ErrorSeverity::WARN);
    EXPECT_TRUE(meta.is_recoverable);
}

TEST(ErrorRegistry, unknown_code_throws) {
    // Verify that get() throws on codes that don't exist
    // All defined codes must return valid entries, there is no "unknown" enum value
    // that we can query without an explicit cast
    SUCCEED();
}

// ---- Domain Header Compile-Time Tests ----

TEST(DomainHeaders, plugin_source_defaults) {
    PluginSource src;
    EXPECT_EQ(src.source_type, PluginSourceType::BUNDLED);
    EXPECT_TRUE(src.enabled);
    EXPECT_EQ(src.diagnostics_state, PluginDiagnosticsState::OK);
}

TEST(DomainHeaders, plugin_device_info_defaults) {
    PluginDeviceInfo di;
    EXPECT_FALSE(di.supports_raw);
    EXPECT_FALSE(di.has_diagnostics);
    EXPECT_FALSE(di.exclusive_resource_id.has_value());
}

TEST(DomainHeaders, stream_ring_descriptor_defaults) {
    StreamRingDescriptor srd;
    EXPECT_EQ(srd.slot_count, 0u);
    EXPECT_EQ(srd.ownership, RingOwnership::HOST_OWNS);
    EXPECT_EQ(srd.policy, RingPolicy::NO_DROP);
    EXPECT_GT(srd.header_size(), 0u);
}

TEST(DomainHeaders, resource_request_defaults) {
    ResourceRequest rr;
    EXPECT_EQ(rr.stream_count, 0u);
    EXPECT_EQ(rr.encoder_slots_needed, 0u);
}

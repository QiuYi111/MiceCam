#include "OAKPluginServer.h"

#include <set>
#include <sstream>

#include <spdlog/spdlog.h>

namespace micecam::plugin {

OAKPluginServer::OAKPluginServer()
    : start_time_(std::chrono::steady_clock::now()),
      hardware_available_(false),
      sdk_available_(false) {
    spdlog::info("OAKPluginServer created (sdk={}, hardware={})",
                 sdk_available_ ? "yes" : "no",
                 hardware_available_ ? "yes" : "no");
}

OAKPluginServer::~OAKPluginServer() = default;

grpc::Status OAKPluginServer::Handshake(
    grpc::ServerContext*,
    const HandshakeRequest* req,
    HandshakeResponse* resp) {
    spdlog::info("Handshake: MiceCam v{} (api v{})",
                 req->micecam_version(), req->plugin_api_version());

    resp->set_plugin_version(kPluginVersion);
    resp->set_plugin_name(kPluginName);

    if (req->plugin_api_version() != kApiVersion) {
        resp->set_accepted(false);
        resp->set_negotiated_api_version(
            std::min(req->plugin_api_version(), kApiVersion));
        auto* w = resp->add_warnings();
        *w = "API version mismatch: host=" + std::to_string(req->plugin_api_version())
           + " plugin=" + std::to_string(kApiVersion);
    } else {
        resp->set_accepted(true);
        resp->set_negotiated_api_version(kApiVersion);
    }

    if (!sdk_available_) {
        auto* w = resp->add_warnings();
        *w = "DepthAI SDK not available: device enumeration and streaming disabled";
    }
    if (!hardware_available_) {
        auto* w = resp->add_warnings();
        *w = "No OAK hardware detected: running in diagnostic-only mode";
    }

    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::GetPluginInfo(
    grpc::ServerContext*,
    const GetPluginInfoRequest*,
    PluginInfo* resp) {
    resp->set_id("micecam.oak");
    resp->set_name(kPluginName);
    resp->set_version(kPluginVersion);
    resp->set_plugin_api_version(kApiVersion);
    resp->set_min_micecam_version(kMinMicecamVersion);
    resp->set_preferred_process_model(ProcessModel::SINGLETON);
    resp->add_supported_process_models(ProcessModel::SINGLETON);
    resp->add_supported_process_models(ProcessModel::PER_DEVICE);
    resp->add_optional_features("depthai_sdk");
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::EnumerateDevices(
    grpc::ServerContext*,
    const EnumerateDevicesRequest*,
    EnumerateDevicesResponse* resp) {
    if (!sdk_available_) {
        auto* diag = resp->add_devices();
        diag->set_device_id("__oak_diagnostic__");
        diag->set_display_name("OAK Diagnostic (SDK unavailable)");
        diag->set_has_persistent_id(false);
        diag->set_vendor("MiceCam");
        auto* si = diag->add_streams();
        si->set_stream_index(0);
        si->set_label("diagnostic");
        si->set_available(false);
        si->set_unavailable_reason(
            "DepthAI SDK not found. Install depthai-core to enable OAK device enumeration.");
        spdlog::info("EnumerateDevices: SDK unavailable, returning diagnostic entry");
        return grpc::Status::OK;
    }

    if (!hardware_available_) {
        auto* diag = resp->add_devices();
        diag->set_device_id("__oak_diagnostic__");
        diag->set_display_name("OAK Diagnostic (no hardware)");
        diag->set_has_persistent_id(false);
        diag->set_vendor("MiceCam");
        auto* si = diag->add_streams();
        si->set_stream_index(0);
        si->set_label("diagnostic");
        si->set_available(false);
        si->set_unavailable_reason(
            "No OAK device detected. Connect an OAK-D or OAK-D-Lite and retry.");
        spdlog::info("EnumerateDevices: no hardware, returning diagnostic entry");
        return grpc::Status::OK;
    }

    spdlog::info("EnumerateDevices: hardware present but skeleton has no DepthAI scan yet");
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::GetCapabilities(
    grpc::ServerContext*,
    const GetCapabilitiesRequest*,
    CapabilityInfo* resp) {
    resp->set_device_id("");
    resp->set_stream_index(0);
    resp->set_supports_raw(false);
    resp->set_supports_mjpeg(false);
    resp->set_supports_h264(false);
    resp->set_supports_h265(false);
    resp->set_max_width(0);
    resp->set_max_height(0);
    resp->set_max_framerate(0.0);
    resp->set_plugin_metadata(
        "{\"status\":\"unavailable\",\"reason\":\"no_oak_hardware_or_sdk\"}");
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::GetConfigSchema(
    grpc::ServerContext*,
    const GetConfigSchemaRequest*,
    ConfigSchema* resp) {
    {
        auto* f = resp->add_fields();
        f->set_key("encoder_profile");
        f->set_display_name("Encoder Profile");
        f->set_description("H264/H265 encoding profile for OAK on-device encoder");
        f->set_type(ConfigFieldType::ENUM);
        f->set_default_value("H264");
        f->add_enum_values("H264");
        f->add_enum_values("H265");
        f->set_required(false);
        f->set_apply_mode(ApplyMode::PRE_OPEN);
    }
    {
        auto* f = resp->add_fields();
        f->set_key("resolution");
        f->set_display_name("Resolution Preset");
        f->set_description("Capture resolution preset");
        f->set_type(ConfigFieldType::ENUM);
        f->set_default_value("1080p");
        f->add_enum_values("720p");
        f->add_enum_values("1080p");
        f->add_enum_values("4K");
        f->set_required(false);
        f->set_apply_mode(ApplyMode::PRE_OPEN);
    }
    {
        auto* f = resp->add_fields();
        f->set_key("framerate");
        f->set_display_name("Framerate");
        f->set_description("Capture framerate in FPS");
        f->set_type(ConfigFieldType::INT);
        f->set_default_value("30");
        f->set_min_value("1");
        f->set_max_value("60");
        f->set_required(false);
        f->set_apply_mode(ApplyMode::PRE_OPEN);
    }
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::ValidateConfig(
    grpc::ServerContext*,
    const ValidateConfigRequest* req,
    ValidateConfigResponse* resp) {
    static const std::set<std::string> kValidEncoderProfiles = {"H264", "H265"};
    static const std::set<std::string> kValidResolutions = {"720p", "1080p", "4K"};

    std::vector<std::string> errors;
    const auto& config = req->config();

    for (const auto& [key, value] : config) {
        if (key == "encoder_profile") {
            if (!kValidEncoderProfiles.count(value)) {
                errors.push_back("invalid encoder_profile: " + value
                                 + " (expected H264 or H265)");
            }
        } else if (key == "resolution") {
            if (!kValidResolutions.count(value)) {
                errors.push_back("invalid resolution: " + value
                                 + " (expected 720p, 1080p, or 4K)");
            }
        } else if (key == "framerate") {
            try {
                int fps = std::stoi(value);
                if (fps < 1 || fps > 60) {
                    errors.push_back("framerate out of range [1, 60]");
                }
            } catch (...) {
                errors.push_back("framerate must be an integer");
            }
        } else {
            errors.push_back("unknown config key: " + key);
        }
    }

    resp->set_valid(errors.empty());
    for (auto& e : errors) {
        resp->add_errors(e);
    }
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::OpenStream(
    grpc::ServerContext*,
    const OpenStreamRequest*,
    OpenStreamResponse* resp) {
    auto* err = resp->mutable_error();
    err->set_error_code("OAK_UNAVAILABLE");
    err->set_severity(ErrorSeverity::WARN);
    err->set_is_recoverable(true);
    err->set_user_message("OAK streaming is not available without hardware and SDK");
    err->set_technical_detail(
        "OpenStream rejected: hardware_available=" + std::string(hardware_available_ ? "true" : "false")
        + " sdk_available=" + std::string(sdk_available_ ? "true" : "false"));
    err->set_suggested_action("Connect OAK hardware and ensure DepthAI SDK is installed");
    err->set_recovery_action(RecoveryAction::RETRY);
    err->set_retry_delay_ms(5000);
    resp->set_success(false);
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::StartStream(
    grpc::ServerContext*,
    const StartStreamRequest*,
    StartStreamResponse* resp) {
    auto* err = resp->mutable_error();
    err->set_error_code("OAK_UNAVAILABLE");
    err->set_severity(ErrorSeverity::WARN);
    err->set_is_recoverable(true);
    err->set_user_message("OAK streaming is not available without hardware and SDK");
    resp->set_success(false);
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::StopStream(
    grpc::ServerContext*,
    const StopStreamRequest*,
    StopStreamResponse* resp) {
    auto* err = resp->mutable_error();
    err->set_error_code("OAK_UNAVAILABLE");
    err->set_severity(ErrorSeverity::WARN);
    err->set_is_recoverable(true);
    err->set_user_message("OAK streaming is not available without hardware and SDK");
    resp->set_success(false);
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::Shutdown(
    grpc::ServerContext*,
    const ShutdownRequest*,
    ShutdownResponse* resp) {
    spdlog::info("Shutdown requested");
    resp->set_success(true);
    return grpc::Status::OK;
}

grpc::Status OAKPluginServer::HealthCheck(
    grpc::ServerContext*,
    const HealthCheckRequest*,
    HealthStatus* resp) {
    resp->set_healthy(true);

    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    auto uptime_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

    std::ostringstream msg;
    msg << "uptime=" << uptime_sec
        << "s oak_sdk=" << (sdk_available_ ? "ok" : "unavailable")
        << " oak_hardware=" << (hardware_available_ ? "ok" : "unavailable")
        << " streams=0";
    resp->set_status_message(msg.str());

    return grpc::Status::OK;
}

} // namespace micecam::plugin

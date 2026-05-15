#include "FFmpegPluginServer.h"

#include <algorithm>
#include <sstream>

#include <spdlog/spdlog.h>

namespace micecam::plugin {

FFmpegPluginServer::FFmpegPluginServer()
    : start_time_(std::chrono::steady_clock::now()) {
    spdlog::info("FFmpegPluginServer created");
}

FFmpegPluginServer::~FFmpegPluginServer() {
    // Shutdown all streams
    std::lock_guard<std::mutex> lock(streams_mutex_);
    if (!streams_.empty()) {
        spdlog::info("FFmpegPluginServer: releasing {} streams on shutdown", streams_.size());
    }
    for (auto& [id, stream] : streams_) {
        if (stream->ring) {
            stream->ring->release();
        }
    }
    streams_.clear();
}

grpc::Status FFmpegPluginServer::Handshake(
    grpc::ServerContext*,
    const HandshakeRequest* req,
    HandshakeResponse* resp) {
    spdlog::info("Handshake: MiceCam v{} (api v{})",
                 req->micecam_version(), req->plugin_api_version());

    resp->set_plugin_version(kPluginVersion);
    resp->set_plugin_name(kPluginName);

    if (req->plugin_api_version() != kApiVersion) {
        resp->set_accepted(false);
        resp->set_negotiated_api_version(std::min(req->plugin_api_version(), kApiVersion));
        auto* w = resp->add_warnings();
        *w = "API version mismatch: host=" + std::to_string(req->plugin_api_version())
           + " plugin=" + std::to_string(kApiVersion);
    } else {
        resp->set_accepted(true);
        resp->set_negotiated_api_version(kApiVersion);
    }

    if (req->micecam_version() < kMinMicecamVersion) {
        auto* w = resp->add_warnings();
        *w = "MiceCam host version " + req->micecam_version()
           + " is below minimum " + kMinMicecamVersion;
    }

    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::GetPluginInfo(
    grpc::ServerContext*,
    const GetPluginInfoRequest*,
    PluginInfo* resp) {
    resp->set_id("micecam.ffmpeg");
    resp->set_name(kPluginName);
    resp->set_version(kPluginVersion);
    resp->set_plugin_api_version(kApiVersion);
    resp->set_min_micecam_version(kMinMicecamVersion);
    resp->set_preferred_process_model(ProcessModel::PER_DEVICE);
    resp->add_supported_process_models(ProcessModel::SINGLETON);
    resp->add_supported_process_models(ProcessModel::PER_DEVICE);
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::EnumerateDevices(
    grpc::ServerContext*,
    const EnumerateDevicesRequest*,
    EnumerateDevicesResponse* resp) {
    ensureDevicesCached();
    for (const auto& dev : cached_devices_) {
        auto* di = resp->add_devices();
        di->set_device_id(dev.device_id);
        di->set_display_name(dev.display_name);
        di->set_persistent_id(dev.persistent_id);
        di->set_vendor(dev.vendor);
        di->set_serial(dev.serial);
        di->set_has_persistent_id(dev.has_persistent_id);

        // Add default stream info
        auto* si = di->add_streams();
        si->set_stream_index(0);
        si->set_label(dev.display_name);
        si->set_max_width(dev.max_width);
        si->set_max_height(dev.max_height);
        si->set_max_framerate(dev.max_framerate);
        si->set_available(dev.available);
        si->set_unavailable_reason(dev.unavailable_reason);
        si->add_supported_payloads(PayloadKind::RAW);
        si->add_supported_payloads(PayloadKind::MJPEG);
#ifdef __APPLE__
        si->add_supported_payloads(PayloadKind::H264);
#endif
        si->add_supported_pixel_formats("yuv420p");
        si->add_supported_pixel_formats("nv12");
        si->add_supported_pixel_formats("mjpeg");
    }

    spdlog::info("EnumerateDevices: found {} device(s)", cached_devices_.size());
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::GetCapabilities(
    grpc::ServerContext*,
    const GetCapabilitiesRequest* req,
    CapabilityInfo* resp) {
    if (!validateDeviceId(req->device_id())) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "device not found: " + req->device_id());
    }

    *resp = buildCapabilityInfo(req->device_id());
    resp->set_stream_index(req->stream_index());
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::GetConfigSchema(
    grpc::ServerContext*,
    const GetConfigSchemaRequest*,
    ConfigSchema* resp) {
    // Resolution config
    {
        auto* f = resp->add_fields();
        f->set_key("width");
        f->set_display_name("Width");
        f->set_description("Capture width in pixels");
        f->set_type(ConfigFieldType::INT);
        f->set_default_value("1920");
        f->set_min_value("320");
        f->set_max_value("4096");
        f->set_required(false);
        f->set_apply_mode(ApplyMode::PRE_OPEN);
    }
    {
        auto* f = resp->add_fields();
        f->set_key("height");
        f->set_display_name("Height");
        f->set_description("Capture height in pixels");
        f->set_type(ConfigFieldType::INT);
        f->set_default_value("1080");
        f->set_min_value("240");
        f->set_max_value("2160");
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
        f->set_max_value("120");
        f->set_required(false);
        f->set_apply_mode(ApplyMode::PRE_OPEN);
    }
    {
        auto* f = resp->add_fields();
        f->set_key("pixel_format");
        f->set_display_name("Pixel Format");
        f->set_description("Capture pixel format");
        f->set_type(ConfigFieldType::ENUM);
        f->set_default_value("yuv420p");
        f->add_enum_values("yuv420p");
        f->add_enum_values("nv12");
        f->add_enum_values("mjpeg");
        f->set_required(false);
        f->set_apply_mode(ApplyMode::PRE_OPEN);
    }
    {
        auto* f = resp->add_fields();
        f->set_key("payload_kind");
        f->set_display_name("Payload Kind");
        f->set_description("Ring payload encoding");
        f->set_type(ConfigFieldType::ENUM);
        f->set_default_value("RAW");
        f->add_enum_values("RAW");
        f->add_enum_values("MJPEG");
        f->set_required(false);
        f->set_apply_mode(ApplyMode::PRE_OPEN);
    }
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::ValidateConfig(
    grpc::ServerContext*,
    const ValidateConfigRequest* req,
    ValidateConfigResponse* resp) {
    std::vector<std::string> errors;
    const auto& config = req->config();

    if (config.count("width")) {
        try {
            int w = std::stoi(config.at("width"));
            if (w < 320 || w > 4096) errors.push_back("width out of range [320, 4096]");
        } catch (...) {
            errors.push_back("width must be an integer");
        }
    }
    if (config.count("height")) {
        try {
            int h = std::stoi(config.at("height"));
            if (h < 240 || h > 2160) errors.push_back("height out of range [240, 2160]");
        } catch (...) {
            errors.push_back("height must be an integer");
        }
    }
    if (config.count("framerate")) {
        try {
            int fps = std::stoi(config.at("framerate"));
            if (fps < 1 || fps > 120) errors.push_back("framerate out of range [1, 120]");
        } catch (...) {
            errors.push_back("framerate must be an integer");
        }
    }
    if (config.count("pixel_format")) {
        static const std::set<std::string> valid_pf = {"yuv420p", "nv12", "mjpeg"};
        if (!valid_pf.count(config.at("pixel_format"))) {
            errors.push_back("invalid pixel_format: " + config.at("pixel_format"));
        }
    }
    if (config.count("payload_kind")) {
        static const std::set<std::string> valid_pk = {"RAW", "MJPEG", "H264", "H265"};
        if (!valid_pk.count(config.at("payload_kind"))) {
            errors.push_back("invalid payload_kind: " + config.at("payload_kind"));
        }
    }

    resp->set_valid(errors.empty());
    for (auto& e : errors) {
        resp->add_errors(e);
    }
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::OpenStream(
    grpc::ServerContext*,
    const OpenStreamRequest* req,
    OpenStreamResponse* resp) {
    const auto& config = req->config();

    if (!validateDeviceId(config.device_id())) {
        auto* err = resp->mutable_error();
        err->set_error_code("DEVICE_UNAVAILABLE");
        err->set_severity(ErrorSeverity::ERROR);
        err->set_is_recoverable(true);
        err->set_user_message("Device not found: " + config.device_id());
        resp->set_success(false);
        return grpc::Status::OK;
    }

    uint32_t slot_count = req->ring_slot_count();
    uint32_t slot_size = req->ring_slot_size();
    if (slot_count == 0) slot_count = 16;
    if (slot_size == 0) slot_size = 256 * 1024; // 256 KB default

    std::string stream_id = generateStreamId();
    std::string ring_id = "ring_" + stream_id;

    auto stream = std::make_unique<ActiveStream>();
    stream->stream_id = stream_id;
    stream->device_id = config.device_id();
    stream->stream_index = config.stream_index();
    stream->width = config.width();
    stream->height = config.height();
    stream->framerate = config.framerate();
    stream->pixel_format = config.pixel_format();
    stream->payload_kind = config.requested_payload();
    stream->ring = std::make_unique<RingFrameProducer>();

    if (!stream->ring->create(ring_id, slot_count, slot_size)) {
        auto* err = resp->mutable_error();
        err->set_error_code("SHM_ALLOC_FAILED");
        err->set_severity(ErrorSeverity::FATAL);
        err->set_is_recoverable(false);
        err->set_user_message("Failed to allocate shared memory ring");
        err->set_technical_detail("shm_open failed for " + ring_id);
        resp->set_success(false);
        return grpc::Status::OK;
    }

    auto ring_desc = stream->ring->descriptor(stream_id);
    auto* rd = resp->mutable_ring_descriptor();
    rd->set_ring_id(ring_desc.ring_id);
    rd->set_stream_id(ring_desc.stream_id);
    rd->set_slot_count(ring_desc.slot_count);
    rd->set_slot_size(ring_desc.slot_size);
    rd->set_platform_handle_type(ring_desc.platform_handle_type);
    rd->set_platform_handle_value(
        std::string(reinterpret_cast<const char*>(&ring_desc.platform_handle_value),
                    sizeof(ring_desc.platform_handle_value)));
    rd->set_ownership(RingOwnership::PLUGIN_OWNS);
    rd->set_policy(RingPolicy::NO_DROP);
    rd->set_producer_sequence_offset(ring_desc.producer_sequence_offset);
    rd->set_consumer_sequence_offset(ring_desc.consumer_sequence_offset);
    rd->set_header_size(RingFrameProducer::kHeaderSize);
    rd->set_payload_offset(ring_desc.payload_offset);

    {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        streams_[stream_id] = std::move(stream);
    }

    resp->set_success(true);
    spdlog::info("OpenStream: {} (device={}, {}x{}@{}, {} slots x {}B)",
                 stream_id, config.device_id(), config.width(), config.height(),
                 config.framerate(), slot_count, slot_size);
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::StartStream(
    grpc::ServerContext*,
    const StartStreamRequest* req,
    StartStreamResponse* resp) {
    std::lock_guard<std::mutex> lock(streams_mutex_);
    auto it = streams_.find(req->stream_id());
    if (it == streams_.end()) {
        auto* err = resp->mutable_error();
        err->set_error_code("STREAM_OPEN_FAILED");
        err->set_user_message("Stream not found: " + req->stream_id());
        resp->set_success(false);
        return grpc::Status::OK;
    }

    // In Phase 2, we don't start real capture — just mark as started
    // Phase 6 (HIL) will add real frame capture with AVFoundation
    it->second->started = true;
    resp->set_success(true);
    spdlog::info("StartStream: {} marked as started", req->stream_id());
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::StopStream(
    grpc::ServerContext*,
    const StopStreamRequest* req,
    StopStreamResponse* resp) {
    std::lock_guard<std::mutex> lock(streams_mutex_);
    auto it = streams_.find(req->stream_id());
    if (it == streams_.end()) {
        auto* err = resp->mutable_error();
        err->set_error_code("STREAM_WRITE_FAILED");
        err->set_user_message("Stream not found: " + req->stream_id());
        resp->set_success(false);
        return grpc::Status::OK;
    }

    it->second->started = false;
    if (it->second->ring) {
        it->second->ring->release();
    }
    streams_.erase(it);
    resp->set_success(true);
    spdlog::info("StopStream: {} stopped", req->stream_id());
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::Shutdown(
    grpc::ServerContext*,
    const ShutdownRequest*,
    ShutdownResponse* resp) {
    spdlog::info("Shutdown requested");

    std::lock_guard<std::mutex> lock(streams_mutex_);
    for (auto& [id, stream] : streams_) {
        stream->started = false;
        if (stream->ring) {
            stream->ring->release();
        }
    }
    streams_.clear();

    resp->set_success(true);
    return grpc::Status::OK;
}

grpc::Status FFmpegPluginServer::HealthCheck(
    grpc::ServerContext*,
    const HealthCheckRequest*,
    HealthStatus* resp) {
    resp->set_healthy(true);

    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    auto uptime_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

    std::lock_guard<std::mutex> lock(streams_mutex_);
    size_t active_count = 0;
    size_t started_count = 0;
    for (const auto& [id, stream] : streams_) {
        if (stream->healthy) active_count++;
        if (stream->started) started_count++;
    }

    std::ostringstream msg;
    msg << "uptime=" << uptime_sec
        << "s active_streams=" << active_count
        << " started=" << started_count
        << " avfoundation=" << (enumerator_.isAvailable() ? "ok" : "unavailable");
    resp->set_status_message(msg.str());

    return grpc::Status::OK;
}

CapabilityInfo FFmpegPluginServer::buildCapabilityInfo(const std::string& device_id) const {
    CapabilityInfo ci;
    ci.set_device_id(device_id);
    ci.set_stream_index(0);
    ci.set_supports_raw(true);
    ci.set_supports_mjpeg(true);
    ci.set_supports_h264(false);
    ci.set_supports_h265(false);
    ci.set_max_width(1920);
    ci.set_max_height(1080);
    ci.set_max_framerate(30.0);
    ci.add_acquisition_controls("brightness");
    ci.add_acquisition_controls("contrast");
    ci.add_acquisition_controls("saturation");
    return ci;
}

void FFmpegPluginServer::ensureDevicesCached() const {
    if (devices_cached_) return;
    cached_devices_ = enumerator_.enumerate();
    if (cached_devices_.empty()) {
        EnumeratedDevice synth;
        synth.device_id = "0";
        synth.display_name = "Synthetic Camera (headless fallback)";
        synth.persistent_id = "synthetic_0";
        synth.vendor = "MiceCam";
        synth.has_persistent_id = true;
        synth.max_width = 1920;
        synth.max_height = 1080;
        synth.max_framerate = 30.0;
        synth.available = true;
        cached_devices_.push_back(std::move(synth));
        spdlog::info("ensureDevicesCached: no physical devices, using synthetic fallback");
    }
    devices_cached_ = true;
}

std::string FFmpegPluginServer::generateStreamId() {
    uint64_t id = next_stream_id_++;
    return "stream_" + std::to_string(id);
}

bool FFmpegPluginServer::validateDeviceId(const std::string& device_id) const {
    ensureDevicesCached();
    return std::any_of(cached_devices_.begin(), cached_devices_.end(),
                       [&](const auto& d) { return d.device_id == device_id; });
}

} // namespace micecam::plugin

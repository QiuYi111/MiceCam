#pragma once

#include <atomic>
#include <cstdint>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "micecam/camera_plugin.grpc.pb.h"
#include "AVFoundationEnumerator.h"
#include "RingFrameProducer.h"

namespace micecam::plugin {

struct ActiveStream {
    std::string stream_id;
    std::string device_id;
    int32_t stream_index = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t framerate = 0;
    std::string pixel_format;
    micecam::plugin::PayloadKind payload_kind = micecam::plugin::PayloadKind::RAW;
    std::unique_ptr<RingFrameProducer> ring;
    bool started = false;
    bool healthy = true;
};

class FFmpegPluginServer final : public micecam::plugin::CameraPluginService::Service {
public:
    FFmpegPluginServer();
    ~FFmpegPluginServer() override;

    grpc::Status Handshake(grpc::ServerContext* ctx,
                           const micecam::plugin::HandshakeRequest* req,
                           micecam::plugin::HandshakeResponse* resp) override;

    grpc::Status GetPluginInfo(grpc::ServerContext* ctx,
                               const micecam::plugin::GetPluginInfoRequest* req,
                               micecam::plugin::PluginInfo* resp) override;

    grpc::Status EnumerateDevices(grpc::ServerContext* ctx,
                                  const micecam::plugin::EnumerateDevicesRequest* req,
                                  micecam::plugin::EnumerateDevicesResponse* resp) override;

    grpc::Status GetCapabilities(grpc::ServerContext* ctx,
                                 const micecam::plugin::GetCapabilitiesRequest* req,
                                 micecam::plugin::CapabilityInfo* resp) override;

    grpc::Status GetConfigSchema(grpc::ServerContext* ctx,
                                 const micecam::plugin::GetConfigSchemaRequest* req,
                                 micecam::plugin::ConfigSchema* resp) override;

    grpc::Status ValidateConfig(grpc::ServerContext* ctx,
                                const micecam::plugin::ValidateConfigRequest* req,
                                micecam::plugin::ValidateConfigResponse* resp) override;

    grpc::Status OpenStream(grpc::ServerContext* ctx,
                            const micecam::plugin::OpenStreamRequest* req,
                            micecam::plugin::OpenStreamResponse* resp) override;

    grpc::Status StartStream(grpc::ServerContext* ctx,
                             const micecam::plugin::StartStreamRequest* req,
                             micecam::plugin::StartStreamResponse* resp) override;

    grpc::Status StopStream(grpc::ServerContext* ctx,
                            const micecam::plugin::StopStreamRequest* req,
                            micecam::plugin::StopStreamResponse* resp) override;

    grpc::Status Shutdown(grpc::ServerContext* ctx,
                          const micecam::plugin::ShutdownRequest* req,
                          micecam::plugin::ShutdownResponse* resp) override;

    grpc::Status HealthCheck(grpc::ServerContext* ctx,
                             const micecam::plugin::HealthCheckRequest* req,
                             micecam::plugin::HealthStatus* resp) override;

private:
    static constexpr const char* kPluginVersion = "1.0.0";
    static constexpr const char* kPluginName = "MiceCam FFmpeg Capture";
    static constexpr uint32_t kApiVersion = 1;
    static constexpr const char* kMinMicecamVersion = "2.0.0";

    AVFoundationEnumerator enumerator_;
    mutable std::vector<EnumeratedDevice> cached_devices_;
    mutable bool devices_cached_ = false;
    std::chrono::steady_clock::time_point start_time_;
    std::mutex streams_mutex_;
    std::unordered_map<std::string, std::unique_ptr<ActiveStream>> streams_;
    std::atomic<uint64_t> next_stream_id_{1};

    micecam::plugin::CapabilityInfo buildCapabilityInfo(const std::string& device_id) const;
    std::string generateStreamId();

    void ensureDevicesCached() const;
    bool validateDeviceId(const std::string& device_id) const;
};

} // namespace micecam::plugin

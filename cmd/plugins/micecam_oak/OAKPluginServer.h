#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "micecam/camera_plugin.grpc.pb.h"

namespace micecam::plugin {

class OAKPluginServer final : public micecam::plugin::CameraPluginService::Service {
public:
    OAKPluginServer();
    ~OAKPluginServer() override;

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

    grpc::Status Calibrate(grpc::ServerContext* ctx,
                           const micecam::plugin::CalibrateRequest* req,
                           micecam::plugin::CalibrateResponse* resp) override;

    grpc::Status NotifyStreamStall(grpc::ServerContext* ctx,
                                   const micecam::plugin::NotifyStreamStallRequest* req,
                                   micecam::plugin::NotifyStreamStallResponse* resp) override;

private:
    static constexpr const char* kPluginVersion = "0.1.0";
    static constexpr const char* kPluginName = "MiceCam OAK-D Capture";
    static constexpr uint32_t kApiVersion = 2;
    static constexpr const char* kMinMicecamVersion = "2.0.0";

    std::chrono::steady_clock::time_point start_time_;
    bool hardware_available_ = false;
    bool sdk_available_ = false;
};

} // namespace micecam::plugin

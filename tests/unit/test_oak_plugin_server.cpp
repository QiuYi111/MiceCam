#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>
#include <chrono>
#include <thread>

#include "micecam/camera_plugin.grpc.pb.h"
#include "OAKPluginServer.h"

namespace {

constexpr int kTestTimeoutMs = 3000;

class OAKPluginServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_thread_ = std::make_unique<std::jthread>([this](std::stop_token /*st*/) {
            grpc::ServerBuilder builder;
            std::string addr = "localhost:0";
            builder.AddListeningPort(addr, grpc::InsecureServerCredentials(), &port_);
            builder.RegisterService(&service_);
            server_ = builder.BuildAndStart();
            if (server_) {
                server_->Wait();
            }
        });

        int retries = 50;
        while (port_ == 0 && retries-- > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        ASSERT_GT(port_, 0) << "Server failed to start within timeout";

        channel_ = grpc::CreateChannel("localhost:" + std::to_string(port_),
                                       grpc::InsecureChannelCredentials());
        stub_ = micecam::plugin::CameraPluginService::NewStub(channel_);
    }

    void TearDown() override {
        if (server_) {
            server_->Shutdown();
        }
        server_thread_.reset();
    }

    micecam::plugin::OAKPluginServer service_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<std::jthread> server_thread_;
    int port_ = 0;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<micecam::plugin::CameraPluginService::Stub> stub_;
};

// ---- Handshake Tests ----

TEST_F(OAKPluginServerTest, HandshakeAccepted) {
    micecam::plugin::HandshakeRequest req;
    req.set_micecam_version("2.0.0");
    req.set_plugin_api_version(1);

    micecam::plugin::HandshakeResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->Handshake(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(resp.accepted());
    EXPECT_EQ(resp.negotiated_api_version(), 1u);
    EXPECT_EQ(resp.plugin_version(), "0.1.0");
    EXPECT_EQ(resp.plugin_name(), "MiceCam OAK-D Capture");
}

TEST_F(OAKPluginServerTest, HandshakeVersionMismatch) {
    micecam::plugin::HandshakeRequest req;
    req.set_micecam_version("2.0.0");
    req.set_plugin_api_version(99);

    micecam::plugin::HandshakeResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->Handshake(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.accepted());
    EXPECT_GT(resp.warnings_size(), 0);
}

TEST_F(OAKPluginServerTest, HandshakeLowMicecamVersion) {
    micecam::plugin::HandshakeRequest req;
    req.set_micecam_version("1.0.0");
    req.set_plugin_api_version(1);

    micecam::plugin::HandshakeResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->Handshake(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(resp.accepted());
    EXPECT_GT(resp.warnings_size(), 0);
}

// ---- GetPluginInfo Tests ----

TEST_F(OAKPluginServerTest, GetPluginInfoReturnsCorrectInfo) {
    micecam::plugin::GetPluginInfoRequest req;
    micecam::plugin::PluginInfo resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->GetPluginInfo(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(resp.id(), "micecam.oak");
    EXPECT_EQ(resp.name(), "MiceCam OAK-D Capture");
    EXPECT_EQ(resp.version(), "0.1.0");
    EXPECT_EQ(resp.plugin_api_version(), 1u);
    EXPECT_EQ(resp.min_micecam_version(), "2.0.0");
    EXPECT_EQ(resp.preferred_process_model(), micecam::plugin::ProcessModel::PER_DEVICE);
    EXPECT_GT(resp.supported_process_models_size(), 0);
}

// ---- No-Hardware Diagnostics Tests ----

TEST_F(OAKPluginServerTest, EnumerateDevicesNoHardware) {
    micecam::plugin::EnumerateDevicesRequest req;
    micecam::plugin::EnumerateDevicesResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->EnumerateDevices(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();

    // Without hardware, returns a diagnostic device entry
    EXPECT_EQ(resp.devices_size(), 1);
    const auto& diag_dev = resp.devices(0);
    EXPECT_EQ(diag_dev.device_id(), "__oak_diagnostic__");
    EXPECT_FALSE(diag_dev.has_persistent_id());
}

TEST_F(OAKPluginServerTest, NoHardwareDiagnosticHasStructuredInfo) {
    micecam::plugin::EnumerateDevicesRequest req;
    micecam::plugin::EnumerateDevicesResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->EnumerateDevices(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(resp.devices_size(), 1);

    const auto& diag_dev = resp.devices(0);
    EXPECT_FALSE(diag_dev.display_name().empty());
    ASSERT_GT(diag_dev.streams_size(), 0);
    const auto& si = diag_dev.streams(0);
    EXPECT_FALSE(si.available());
    EXPECT_FALSE(si.unavailable_reason().empty());
}

TEST_F(OAKPluginServerTest, NoHardwareIsStructuredNonFatal) {
    micecam::plugin::EnumerateDevicesRequest req;
    micecam::plugin::EnumerateDevicesResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    // The RPC itself must succeed (non-fatal)
    auto status = stub_->EnumerateDevices(&ctx, req, &resp);
    EXPECT_TRUE(status.ok());
}

// ---- Capabilities Tests ----

TEST_F(OAKPluginServerTest, GetCapabilitiesNoHardware) {
    micecam::plugin::GetCapabilitiesRequest req;
    req.set_device_id("oak_0");
    req.set_stream_index(0);

    micecam::plugin::CapabilityInfo resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->GetCapabilities(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_FALSE(resp.supports_raw());
    EXPECT_FALSE(resp.supports_h264());
    EXPECT_FALSE(resp.supports_h265());
    EXPECT_EQ(resp.max_width(), 0);
    EXPECT_EQ(resp.max_height(), 0);
    EXPECT_FALSE(resp.plugin_metadata().empty());
}

// ---- Config Schema Tests ----

TEST_F(OAKPluginServerTest, GetConfigSchemaReturnsFields) {
    micecam::plugin::GetConfigSchemaRequest req;
    req.set_device_id("oak_0");

    micecam::plugin::ConfigSchema resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->GetConfigSchema(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_GE(resp.fields_size(), 3);
}

// ---- Config Validation Tests ----

TEST_F(OAKPluginServerTest, ValidateConfigValidConfig) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("oak_0");
    (*req.mutable_config())["encoder_profile"] = "H264";
    (*req.mutable_config())["resolution"] = "1080p";
    (*req.mutable_config())["framerate"] = "30";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(resp.valid());
    EXPECT_EQ(resp.errors_size(), 0);
}

TEST_F(OAKPluginServerTest, ValidateConfigInvalidProfile) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("oak_0");
    (*req.mutable_config())["encoder_profile"] = "AV1";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.valid());
    EXPECT_GT(resp.errors_size(), 0);
}

TEST_F(OAKPluginServerTest, ValidateConfigInvalidResolution) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("oak_0");
    (*req.mutable_config())["resolution"] = "8K";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.valid());
    EXPECT_GT(resp.errors_size(), 0);
}

TEST_F(OAKPluginServerTest, ValidateConfigInvalidFramerate) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("oak_0");
    (*req.mutable_config())["framerate"] = "200";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.valid());
    EXPECT_GT(resp.errors_size(), 0);
}

TEST_F(OAKPluginServerTest, ValidateConfigUnknownKey) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("oak_0");
    (*req.mutable_config())["unknown_key"] = "value";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.valid());
    EXPECT_GT(resp.errors_size(), 0);
}

// ---- Stream Lifecycle Unavailable Tests ----

TEST_F(OAKPluginServerTest, OpenStreamUnavailable) {
    micecam::plugin::OpenStreamRequest req;
    auto* config = req.mutable_config();
    config->set_device_id("oak_0");
    config->set_stream_index(0);
    config->set_width(1920);
    config->set_height(1080);
    config->set_framerate(30);
    req.set_ring_slot_count(8);
    req.set_ring_slot_size(65536);

    micecam::plugin::OpenStreamResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->OpenStream(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.success());
    EXPECT_TRUE(resp.has_error());
    EXPECT_FALSE(resp.error().error_code().empty());
    EXPECT_FALSE(resp.error().user_message().empty());
}

TEST_F(OAKPluginServerTest, StartStreamUnavailable) {
    micecam::plugin::StartStreamRequest req;
    req.set_stream_id("stream_test");

    micecam::plugin::StartStreamResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->StartStream(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.success());
    EXPECT_TRUE(resp.has_error());
    EXPECT_FALSE(resp.error().error_code().empty());
}

TEST_F(OAKPluginServerTest, StopStreamUnavailable) {
    micecam::plugin::StopStreamRequest req;
    req.set_stream_id("stream_test");

    micecam::plugin::StopStreamResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->StopStream(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.success());
    EXPECT_TRUE(resp.has_error());
}

// ---- Health Check Tests ----

TEST_F(OAKPluginServerTest, HealthCheckReturnsHealthy) {
    micecam::plugin::HealthCheckRequest req;
    micecam::plugin::HealthStatus resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->HealthCheck(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(resp.healthy());
    EXPECT_FALSE(resp.status_message().empty());
}

TEST_F(OAKPluginServerTest, ShutdownSucceeds) {
    micecam::plugin::ShutdownRequest req;
    micecam::plugin::ShutdownResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->Shutdown(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(resp.success());
}

} // namespace

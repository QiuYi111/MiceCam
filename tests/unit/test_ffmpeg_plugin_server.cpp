#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>
#include <chrono>
#include <thread>

#include "micecam/camera_plugin.grpc.pb.h"
#include "FFmpegPluginServer.h"

namespace {

constexpr int kTestTimeoutMs = 3000;

class FFmpegPluginServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_thread_ = std::make_unique<std::jthread>([this](std::stop_token /*st*/) {
            grpc::ServerBuilder builder;
            std::string addr = "localhost:0";
            builder.AddListeningPort(addr, grpc::InsecureServerCredentials(), &port_);
            builder.RegisterService(&service_);
            server_ = builder.BuildAndStart();
            if (server_) {
                // Wait until stopped
                server_->Wait();
            }
        });

        // Wait for server to start
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

    micecam::plugin::FFmpegPluginServer service_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<std::jthread> server_thread_;
    int port_ = 0;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<micecam::plugin::CameraPluginService::Stub> stub_;
};

TEST_F(FFmpegPluginServerTest, HandshakeAccepted) {
    micecam::plugin::HandshakeRequest req;
    req.set_micecam_version("2.0.0");
    req.set_plugin_api_version(2);

    micecam::plugin::HandshakeResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->Handshake(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(resp.accepted());
    EXPECT_EQ(resp.negotiated_api_version(), 2u);
    EXPECT_EQ(resp.plugin_version(), "1.0.0");
    EXPECT_EQ(resp.plugin_name(), "MiceCam FFmpeg Capture");
}

TEST_F(FFmpegPluginServerTest, HandshakeVersionMismatch) {
    micecam::plugin::HandshakeRequest req;
    req.set_micecam_version("1.0.0");
    req.set_plugin_api_version(99);

    micecam::plugin::HandshakeResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->Handshake(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.accepted());
    EXPECT_EQ(resp.negotiated_api_version(), 2u);
    EXPECT_GT(resp.warnings_size(), 0);
}

TEST_F(FFmpegPluginServerTest, GetPluginInfoReturnsCorrectInfo) {
    micecam::plugin::GetPluginInfoRequest req;
    micecam::plugin::PluginInfo resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->GetPluginInfo(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_EQ(resp.id(), "micecam.ffmpeg");
    EXPECT_EQ(resp.name(), "MiceCam FFmpeg Capture");
    EXPECT_EQ(resp.version(), "1.0.0");
    EXPECT_EQ(resp.plugin_api_version(), 2u);
    EXPECT_EQ(resp.min_micecam_version(), "2.0.0");
    EXPECT_GT(resp.supported_process_models_size(), 0);
}

TEST_F(FFmpegPluginServerTest, EnumerateDevicesReturnsResponse) {
    micecam::plugin::EnumerateDevicesRequest req;
    micecam::plugin::EnumerateDevicesResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->EnumerateDevices(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    // May return 0 devices on headless systems, that's fine
    SUCCEED() << "EnumerateDevices returned " << resp.devices_size() << " device(s)";
}

TEST_F(FFmpegPluginServerTest, GetCapabilitiesReturnsValidInfo) {
    micecam::plugin::GetCapabilitiesRequest req;
    req.set_device_id("0");
    req.set_stream_index(0);

    micecam::plugin::CapabilityInfo resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->GetCapabilities(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(resp.device_id(), "0");
    EXPECT_TRUE(resp.supports_raw());
    EXPECT_GT(resp.max_width(), 0);
    EXPECT_GT(resp.max_height(), 0);
    EXPECT_GT(resp.max_framerate(), 0.0);
}

TEST_F(FFmpegPluginServerTest, GetCapabilitiesInvalidDevice) {
    micecam::plugin::GetCapabilitiesRequest req;
    req.set_device_id("nonexistent");
    req.set_stream_index(0);

    micecam::plugin::CapabilityInfo resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->GetCapabilities(&ctx, req, &resp);
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

TEST_F(FFmpegPluginServerTest, GetConfigSchemaReturnsFields) {
    micecam::plugin::GetConfigSchemaRequest req;
    req.set_device_id("0");

    micecam::plugin::ConfigSchema resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->GetConfigSchema(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_GT(resp.fields_size(), 0);
}

TEST_F(FFmpegPluginServerTest, ValidateConfigValidConfig) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("0");
    (*req.mutable_config())["width"] = "1920";
    (*req.mutable_config())["height"] = "1080";
    (*req.mutable_config())["framerate"] = "30";
    (*req.mutable_config())["pixel_format"] = "yuv420p";
    (*req.mutable_config())["payload_kind"] = "RAW";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(resp.valid());
    EXPECT_EQ(resp.errors_size(), 0);
}

TEST_F(FFmpegPluginServerTest, ValidateConfigInvalidWidth) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("0");
    (*req.mutable_config())["width"] = "10000";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.valid());
    EXPECT_GT(resp.errors_size(), 0);
}

TEST_F(FFmpegPluginServerTest, ValidateConfigInvalidPixelFormat) {
    micecam::plugin::ValidateConfigRequest req;
    req.set_device_id("0");
    (*req.mutable_config())["pixel_format"] = "rgb999";

    micecam::plugin::ValidateConfigResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->ValidateConfig(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.valid());
    EXPECT_GT(resp.errors_size(), 0);
}

TEST_F(FFmpegPluginServerTest, HealthCheckReturnsHealthy) {
    micecam::plugin::HealthCheckRequest req;
    micecam::plugin::HealthStatus resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->HealthCheck(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(resp.healthy());
    EXPECT_FALSE(resp.status_message().empty());
}

TEST_F(FFmpegPluginServerTest, OpenStreamReturnsRingDescriptor) {
    micecam::plugin::OpenStreamRequest req;
    auto* config = req.mutable_config();
    config->set_device_id("0");
    config->set_stream_index(0);
    config->set_width(1920);
    config->set_height(1080);
    config->set_framerate(30);
    config->set_requested_payload(micecam::plugin::PayloadKind::RAW);
    req.set_ring_slot_count(8);
    req.set_ring_slot_size(65536);

    micecam::plugin::OpenStreamResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->OpenStream(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success()) << "error=" << resp.error().user_message();
    EXPECT_GT(resp.ring_descriptor().slot_count(), 0u);
    EXPECT_GT(resp.ring_descriptor().slot_size(), 0u);
    EXPECT_FALSE(resp.ring_descriptor().ring_id().empty());
    EXPECT_FALSE(resp.ring_descriptor().stream_id().empty());
    EXPECT_EQ(resp.ring_descriptor().platform_handle_type(), "posix_shm");
    EXPECT_EQ(resp.ring_descriptor().ownership(),
              micecam::plugin::RingOwnership::PLUGIN_OWNS);
    EXPECT_GT(resp.ring_descriptor().header_size(), 0u);
    EXPECT_GT(resp.ring_descriptor().payload_offset(), 0u);

    // Cleanup: stop the stream
    std::string stream_id = resp.ring_descriptor().stream_id();
    micecam::plugin::StopStreamRequest stop_req;
    stop_req.set_stream_id(stream_id);
    micecam::plugin::StopStreamResponse stop_resp;
    grpc::ClientContext stop_ctx;
    stop_ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));
    auto stop_status = stub_->StopStream(&stop_ctx, stop_req, &stop_resp);
    EXPECT_TRUE(stop_status.ok());
    EXPECT_TRUE(stop_resp.success());
}

TEST_F(FFmpegPluginServerTest, OpenStreamInvalidDevice) {
    micecam::plugin::OpenStreamRequest req;
    auto* config = req.mutable_config();
    config->set_device_id("nonexistent");
    config->set_stream_index(0);
    req.set_ring_slot_count(8);
    req.set_ring_slot_size(65536);

    micecam::plugin::OpenStreamResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->OpenStream(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.success());
    EXPECT_EQ(resp.error().error_code(), "DEVICE_UNAVAILABLE");
}

TEST_F(FFmpegPluginServerTest, StartStopStream) {
    // Open stream first
    micecam::plugin::OpenStreamRequest open_req;
    auto* config = open_req.mutable_config();
    config->set_device_id("0");
    config->set_stream_index(0);
    config->set_width(1920);
    config->set_height(1080);
    config->set_framerate(30);
    open_req.set_ring_slot_count(8);
    open_req.set_ring_slot_size(65536);

    micecam::plugin::OpenStreamResponse open_resp;
    grpc::ClientContext open_ctx;
    open_ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto open_status = stub_->OpenStream(&open_ctx, open_req, &open_resp);
    ASSERT_TRUE(open_status.ok() && open_resp.success());
    std::string stream_id = open_resp.ring_descriptor().stream_id();

    // Start
    {
        micecam::plugin::StartStreamRequest req;
        req.set_stream_id(stream_id);
        micecam::plugin::StartStreamResponse resp;
        grpc::ClientContext ctx;
        ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));
        auto status = stub_->StartStream(&ctx, req, &resp);
        EXPECT_TRUE(status.ok());
        EXPECT_TRUE(resp.success());
    }

    // Stop
    {
        micecam::plugin::StopStreamRequest req;
        req.set_stream_id(stream_id);
        micecam::plugin::StopStreamResponse resp;
        grpc::ClientContext ctx;
        ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));
        auto status = stub_->StopStream(&ctx, req, &resp);
        EXPECT_TRUE(status.ok());
        EXPECT_TRUE(resp.success());
    }
}

TEST_F(FFmpegPluginServerTest, StopStreamInvalidId) {
    micecam::plugin::StopStreamRequest req;
    req.set_stream_id("nonexistent");
    micecam::plugin::StopStreamResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->StopStream(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.success());
}

TEST_F(FFmpegPluginServerTest, ShutdownSucceeds) {
    micecam::plugin::ShutdownRequest req;
    micecam::plugin::ShutdownResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs));

    auto status = stub_->Shutdown(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(resp.success());
}

TEST_F(FFmpegPluginServerTest, CalibrateReturnsNotImplemented) {
    micecam::plugin::CalibrateRequest req;
    req.set_device_id("0");
    req.set_stream_index(0);
    req.set_width(320);
    req.set_height(240);
    req.set_fps(10.0);
    req.set_calibration_duration_ms(500);
    req.set_prefer_hardware_encoder(false);

    micecam::plugin::CalibrateResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(15000));

    auto status = stub_->Calibrate(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(resp.success()) << "error=" << resp.error();
    EXPECT_TRUE(resp.supported());
    EXPECT_GT(resp.i_frame_latency_ns(), 0u);
    EXPECT_GT(resp.p_frame_latency_ns(), 0u);
    EXPECT_GT(resp.max_sustainable_fps(), 0.0);
    EXPECT_GT(resp.recommended_slot_size(), 0u);
    EXPECT_FALSE(resp.actual_encoder_name().empty());
    EXPECT_EQ(resp.actual_width(), 320);
    EXPECT_EQ(resp.actual_height(), 240);
    EXPECT_LT(resp.i_frame_latency_ns(), 1'000'000'000u);
    EXPECT_LT(resp.p_frame_latency_ns(), 1'000'000'000u);
}

// RingFrameProducer standalone test
TEST(RingFrameProducerTest, CreateAndWrite) {
    micecam::plugin::RingFrameProducer ring;
    ASSERT_TRUE(ring.create("test_ring_cw", 4, 8192));

    std::vector<uint8_t> test_data(1024, 0xAB);
    micecam::plugin::FrameData frame;
    frame.data = test_data.data();
    frame.size = static_cast<uint32_t>(test_data.size());
    frame.width = 640;
    frame.height = 480;
    frame.kind = micecam::domain::PayloadKind::RAW;
    frame.pts_ns = 123456789;
    frame.keyframe = true;

    EXPECT_TRUE(ring.writeFrame(frame, 100));
    EXPECT_EQ(ring.producerSeq(), 1u);

    // Write 3 more frames (fills the ring)
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(ring.writeFrame(frame, 100));
    }
    EXPECT_EQ(ring.producerSeq(), 4u);

    // 5th frame should block and timeout (ring full, no consumer)
    EXPECT_FALSE(ring.writeFrame(frame, 200));

    auto desc = ring.descriptor("my_stream");
    EXPECT_EQ(desc.ring_id, "test_ring_cw");
    EXPECT_EQ(desc.stream_id, "my_stream");
    EXPECT_EQ(desc.slot_count, 4u);
    EXPECT_EQ(desc.slot_size, 8192u);
    EXPECT_EQ(desc.platform_handle_type, "posix_shm");
    EXPECT_EQ(desc.producer_sequence_offset, 0u);
    EXPECT_EQ(desc.consumer_sequence_offset, 8u);
    EXPECT_EQ(desc.payload_offset, 64u);

    ring.release();
    EXPECT_FALSE(ring.isValid());
}

TEST(RingFrameProducerTest, DescriptorHasCorrectOffsets) {
    micecam::plugin::RingFrameProducer ring;
    ASSERT_TRUE(ring.create("test_ring_offsets", 2, 4096));

    auto desc = ring.descriptor("stream_42");
    EXPECT_EQ(desc.producer_sequence_offset, 0u);
    EXPECT_EQ(desc.consumer_sequence_offset, 8u);
    EXPECT_EQ(desc.ownership, micecam::domain::RingOwnership::PLUGIN_OWNS);
    EXPECT_EQ(desc.policy, micecam::domain::RingPolicy::NO_DROP);
    EXPECT_GT(desc.header_size(), 0u);

    ring.release();
}

} // namespace

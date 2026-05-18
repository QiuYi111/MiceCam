#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>
#include <chrono>
#include <thread>

#include "micecam/camera_plugin.grpc.pb.h"
#include "FFmpegPluginServer.h"

namespace {

constexpr int kTestTimeoutMs = 5000;

class NegativePluginRpcTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_thread_ = std::make_unique<std::thread>([this] {
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
        if (server_thread_ && server_thread_->joinable()) {
            server_thread_->join();
        }
        server_thread_.reset();
    }

    auto deadline() const {
        return std::chrono::system_clock::now() + std::chrono::milliseconds(kTestTimeoutMs);
    }

    micecam::plugin::FFmpegPluginServer service_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<std::thread> server_thread_;
    int port_ = 0;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<micecam::plugin::CameraPluginService::Stub> stub_;
};

TEST_F(NegativePluginRpcTest, CalibrateZeroWidthReturnsInvalidArgument) {
    micecam::plugin::CalibrateRequest req;
    req.set_device_id("0");
    req.set_stream_index(0);
    req.set_width(0);
    req.set_height(240);
    req.set_fps(10.0);

    micecam::plugin::CalibrateResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(deadline());

    auto status = stub_->Calibrate(&ctx, req, &resp);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(NegativePluginRpcTest, CalibrateZeroHeightReturnsInvalidArgument) {
    micecam::plugin::CalibrateRequest req;
    req.set_device_id("0");
    req.set_stream_index(0);
    req.set_width(320);
    req.set_height(0);
    req.set_fps(10.0);

    micecam::plugin::CalibrateResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(deadline());

    auto status = stub_->Calibrate(&ctx, req, &resp);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(NegativePluginRpcTest, CalibrateNegativeFpsReturnsInvalidArgument) {
    micecam::plugin::CalibrateRequest req;
    req.set_device_id("0");
    req.set_stream_index(0);
    req.set_width(320);
    req.set_height(240);
    req.set_fps(-1.0);

    micecam::plugin::CalibrateResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(deadline());

    auto status = stub_->Calibrate(&ctx, req, &resp);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(NegativePluginRpcTest, CalibrateZeroFpsReturnsInvalidArgument) {
    micecam::plugin::CalibrateRequest req;
    req.set_device_id("0");
    req.set_stream_index(0);
    req.set_width(320);
    req.set_height(240);
    req.set_fps(0.0);

    micecam::plugin::CalibrateResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(deadline());

    auto status = stub_->Calibrate(&ctx, req, &resp);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(NegativePluginRpcTest, OpenStreamTwiceSameDeviceReturnsAlreadyExists) {
    micecam::plugin::OpenStreamRequest req;
    auto* config = req.mutable_config();
    config->set_device_id("0");
    config->set_stream_index(0);
    config->set_width(640);
    config->set_height(480);
    config->set_framerate(15);
    config->set_requested_payload(micecam::plugin::PayloadKind::RAW);
    req.set_ring_slot_count(4);
    req.set_ring_slot_size(8192);

    micecam::plugin::OpenStreamResponse resp1;
    grpc::ClientContext ctx1;
    ctx1.set_deadline(deadline());
    auto status1 = stub_->OpenStream(&ctx1, req, &resp1);
    ASSERT_TRUE(status1.ok()) << status1.error_message();
    ASSERT_TRUE(resp1.success()) << resp1.error().user_message();

    micecam::plugin::OpenStreamResponse resp2;
    grpc::ClientContext ctx2;
    ctx2.set_deadline(deadline());
    auto status2 = stub_->OpenStream(&ctx2, req, &resp2);
    EXPECT_EQ(status2.error_code(), grpc::StatusCode::ALREADY_EXISTS);

    micecam::plugin::StopStreamRequest stop_req;
    stop_req.set_stream_id(resp1.ring_descriptor().stream_id());
    micecam::plugin::StopStreamResponse stop_resp;
    grpc::ClientContext stop_ctx;
    stop_ctx.set_deadline(deadline());
    stub_->StopStream(&stop_ctx, stop_req, &stop_resp);
}

TEST_F(NegativePluginRpcTest, OpenStreamNonexistentDeviceFails) {
    micecam::plugin::OpenStreamRequest req;
    auto* config = req.mutable_config();
    config->set_device_id("nonexistent_device_xyz");
    config->set_stream_index(0);
    config->set_width(640);
    config->set_height(480);
    config->set_framerate(15);
    req.set_ring_slot_count(4);
    req.set_ring_slot_size(8192);

    micecam::plugin::OpenStreamResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(deadline());

    auto status = stub_->OpenStream(&ctx, req, &resp);
    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(resp.success());
    EXPECT_EQ(resp.error().error_code(), "DEVICE_UNAVAILABLE");
}

TEST_F(NegativePluginRpcTest, NotifyStreamStallDisconnectedDeviceNotRecoverable) {
    micecam::plugin::OpenStreamRequest open_req;
    auto* cfg = open_req.mutable_config();
    cfg->set_device_id("0");
    cfg->set_stream_index(0);
    cfg->set_width(640);
    cfg->set_height(480);
    cfg->set_framerate(15);
    cfg->set_requested_payload(micecam::plugin::PayloadKind::RAW);
    open_req.set_ring_slot_count(4);
    open_req.set_ring_slot_size(8192);

    micecam::plugin::OpenStreamResponse open_resp;
    grpc::ClientContext open_ctx;
    open_ctx.set_deadline(deadline());
    ASSERT_TRUE(stub_->OpenStream(&open_ctx, open_req, &open_resp).ok());
    ASSERT_TRUE(open_resp.success());
    std::string stream_id = open_resp.ring_descriptor().stream_id();

    micecam::plugin::NotifyStreamStallRequest stall_req;
    stall_req.set_stream_id(stream_id);
    stall_req.set_stall_duration_ms(5000);

    micecam::plugin::NotifyStreamStallResponse stall_resp;
    grpc::ClientContext stall_ctx;
    stall_ctx.set_deadline(deadline());

    auto status = stub_->NotifyStreamStall(&stall_ctx, stall_req, &stall_resp);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(stall_resp.acknowledged());

    micecam::plugin::StopStreamRequest stop_req;
    stop_req.set_stream_id(stream_id);
    micecam::plugin::StopStreamResponse stop_resp;
    grpc::ClientContext stop_ctx;
    stop_ctx.set_deadline(deadline());
    stub_->StopStream(&stop_ctx, stop_req, &stop_resp);
}

} // namespace

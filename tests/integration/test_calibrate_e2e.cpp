#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>

#include <arpa/inet.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <signal.h>
#include <string>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "micecam/camera_plugin.grpc.pb.h"

namespace {

std::string find_plugin_binary() {
    const char* env = std::getenv("MICECAM_FFMPEG_PLUGIN");
    if (env && env[0] != '\0') return env;

#if defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string self_path(size, '\0');
    _NSGetExecutablePath(self_path.data(), &size);
    auto bin_dir = std::filesystem::path(self_path).parent_path();
#elif defined(__linux__)
    auto bin_dir = std::filesystem::read_symlink("/proc/self/exe").parent_path();
#else
    auto bin_dir = std::filesystem::current_path();
#endif

    auto build_dir = bin_dir.parent_path();
    auto candidate = build_dir / "cmd" / "plugins" / "micecam_ffmpeg" / "micecam_ffmpeg_plugin";
    if (std::filesystem::exists(candidate)) return candidate.string();

    return (std::filesystem::current_path()
            / "cmd/plugins/micecam_ffmpeg/micecam_ffmpeg_plugin")
        .string();
}

int pick_free_port() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);
    if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(sock);
        return -1;
    }
    socklen_t len = sizeof(addr);
    getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len);
    int port = ntohs(addr.sin_port);
    close(sock);
    return port;
}

class ForkedPlugin {
public:
    ~ForkedPlugin() { stop(); }

    bool start(int port) {
        port_ = port;
        auto binary = find_plugin_binary();
        if (!std::filesystem::exists(binary)) return false;
        pid_ = fork();
        if (pid_ < 0) return false;
        if (pid_ == 0) {
            std::string p = "--port=" + std::to_string(port);
            execl(binary.c_str(), "micecam_ffmpeg_plugin", p.c_str(), nullptr);
            _exit(127);
        }
        return true;
    }

    bool wait_ready(int retries = 50, int backoff_ms = 100) {
        channel_ = grpc::CreateChannel(
            "localhost:" + std::to_string(port_),
            grpc::InsecureChannelCredentials());
        for (int i = 0; i < retries; i++) {
            auto deadline = std::chrono::system_clock::now()
                            + std::chrono::milliseconds(backoff_ms);
            if (channel_->WaitForConnected(deadline)) {
                stub_ = micecam::plugin::CameraPluginService::NewStub(channel_);
                return true;
            }
            if (pid_ > 0) {
                int st;
                if (waitpid(pid_, &st, WNOHANG) != 0) return false;
            }
        }
        return false;
    }

    void stop() {
        if (pid_ > 0) {
            ::kill(pid_, SIGTERM);
            int st;
            for (int i = 0; i < 30; i++) {
                if (waitpid(pid_, &st, WNOHANG) != 0) {
                    pid_ = -1;
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            ::kill(pid_, SIGKILL);
            waitpid(pid_, &st, 0);
            pid_ = -1;
        }
    }

    micecam::plugin::CameraPluginService::Stub* stub() {
        return stub_.get();
    }

private:
    pid_t pid_ = -1;
    int port_ = 0;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<micecam::plugin::CameraPluginService::Stub> stub_;
};

bool encoder_name_valid(const std::string& name) {
    return name.find("nvenc") != std::string::npos
        || name.find("videotoolbox") != std::string::npos
        || name.find("qsv") != std::string::npos
        || name.find("libx264") != std::string::npos;
}

} // namespace

class CalibrateE2ETest : public ::testing::Test {
protected:
    ForkedPlugin plugin_;

    void SetUp() override {
        int port = pick_free_port();
        ASSERT_GT(port, 0) << "Failed to find free port";
        ASSERT_TRUE(plugin_.start(port)) << "Failed to fork plugin binary";
        ASSERT_TRUE(plugin_.wait_ready()) << "Plugin did not become ready in time";
    }

    void TearDown() override { plugin_.stop(); }
};

TEST_F(CalibrateE2ETest, CalibrateWithRealEncoder) {
    auto* stub = plugin_.stub();

    micecam::plugin::CalibrateRequest req;
    req.set_device_id("0");
    req.set_stream_index(0);
    req.set_width(640);
    req.set_height(480);
    req.set_fps(30.0);
    req.set_calibration_duration_ms(500);
    req.set_prefer_hardware_encoder(false);

    micecam::plugin::CalibrateResponse resp;
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now()
                     + std::chrono::milliseconds(15000));

    auto status = stub->Calibrate(&ctx, req, &resp);
    ASSERT_TRUE(status.ok()) << status.error_message();

    EXPECT_TRUE(resp.success()) << "Calibration failed: " << resp.error();
    EXPECT_TRUE(resp.supported());

    EXPECT_GT(resp.i_frame_latency_ns(), 0u)
        << "I-frame latency must be positive";
    EXPECT_GT(resp.p_frame_latency_ns(), 0u)
        << "P-frame latency must be positive";
    EXPECT_GT(resp.i_frame_latency_ns(), resp.p_frame_latency_ns())
        << "I-frame latency (" << resp.i_frame_latency_ns()
        << "ns) should exceed P-frame latency (" << resp.p_frame_latency_ns()
        << "ns)";

    EXPECT_GT(resp.max_sustainable_fps(), 10.0)
        << "Max sustainable FPS too low: " << resp.max_sustainable_fps();

    EXPECT_GT(resp.recommended_slot_size(), 0u)
        << "Recommended slot size must be positive";

    EXPECT_FALSE(resp.actual_encoder_name().empty())
        << "Encoder name must not be empty";
    EXPECT_TRUE(encoder_name_valid(resp.actual_encoder_name()))
        << "Unexpected encoder: " << resp.actual_encoder_name();

    EXPECT_EQ(resp.actual_width(), 640);
    EXPECT_EQ(resp.actual_height(), 480);

    micecam::plugin::ShutdownRequest shutdown_req;
    micecam::plugin::ShutdownResponse shutdown_resp;
    grpc::ClientContext shutdown_ctx;
    shutdown_ctx.set_deadline(std::chrono::system_clock::now()
                              + std::chrono::milliseconds(5000));
    stub->Shutdown(&shutdown_ctx, shutdown_req, &shutdown_resp);
}

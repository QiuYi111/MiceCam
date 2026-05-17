#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>

#include <arpa/inet.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
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
        for (auto& n : shm_names_) shm_unlink(n.c_str());
        shm_names_.clear();
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

    void track_shm(const std::string& ring_id) {
        shm_names_.push_back("/micecam_ring_" + ring_id);
    }

    micecam::plugin::CameraPluginService::Stub* stub() {
        return stub_.get();
    }

private:
    pid_t pid_ = -1;
    int port_ = 0;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<micecam::plugin::CameraPluginService::Stub> stub_;
    std::vector<std::string> shm_names_;
};

constexpr uint64_t kRingHeaderSize = 64;
constexpr uint64_t kPayloadHeaderSize = 44;
constexpr auto kTimeout = std::chrono::milliseconds(5000);

struct RingAccessor {
    int fd = -1;
    void* mem = nullptr;
    uint64_t total_size = 0;
    uint32_t slot_count = 0;
    uint32_t slot_size = 0;

    bool open(const std::string& ring_id, uint32_t slots, uint32_t slot_sz) {
        slot_count = slots;
        slot_size = slot_sz;
        total_size = kRingHeaderSize
                     + static_cast<uint64_t>(slots) * slot_sz;
        std::string shm_name = "/micecam_ring_" + ring_id;
        fd = shm_open(shm_name.c_str(), O_RDWR, 0600);
        if (fd < 0) return false;
        mem = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, 0);
        if (mem == MAP_FAILED) {
            close(fd);
            fd = -1;
            mem = nullptr;
            return false;
        }
        return true;
    }

    void close_ring() {
        if (mem) {
            munmap(mem, total_size);
            mem = nullptr;
        }
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    void write_frame(uint64_t seq, const std::vector<uint8_t>& payload,
                     uint32_t kind_val, bool keyframe) {
        uint32_t idx = static_cast<uint32_t>(seq % slot_count);
        uint8_t* slot = static_cast<uint8_t*>(mem) + kRingHeaderSize
                        + static_cast<uint64_t>(idx) * slot_size;
        uint32_t sz = static_cast<uint32_t>(payload.size());
        uint32_t copy_sz = std::min(sz, slot_size - static_cast<uint32_t>(kPayloadHeaderSize));
        uint64_t pts_ns = seq * 33333333ULL;
        uint8_t kf = keyframe ? 1 : 0;
        std::memcpy(slot, &kind_val, 4);
        std::memcpy(slot + 8, &pts_ns, 8);
        std::memcpy(slot + 16, &seq, 8);
        std::memcpy(slot + 24, &kf, 1);
        std::memcpy(slot + 28, &sz, 4);
        int32_t w = 640, h = 480;
        std::memcpy(slot + 32, &w, 4);
        std::memcpy(slot + 36, &h, 4);
        uint32_t checksum = 0;
        const uint32_t* d32 = reinterpret_cast<const uint32_t*>(payload.data());
        uint32_t words = std::min(copy_sz / 4u, 64u);
        for (uint32_t i = 0; i < words; i++) checksum ^= d32[i];
        std::memcpy(slot + 40, &checksum, 4);
        if (copy_sz > 0) {
            std::memcpy(slot + kPayloadHeaderSize, payload.data(), copy_sz);
        }
        auto* header = static_cast<std::atomic<uint64_t>*>(mem);
        header[0].store(seq + 1, std::memory_order_release);
    }

    bool read_keyframe_flag(uint64_t seq) {
        uint32_t idx = static_cast<uint32_t>(seq % slot_count);
        uint8_t* slot = static_cast<uint8_t*>(mem) + kRingHeaderSize
                        + static_cast<uint64_t>(idx) * slot_size;
        uint8_t kf = 0;
        std::memcpy(&kf, slot + 24, 1);
        return kf != 0;
    }
};

} // namespace

class DualPathKeyframeTest : public ::testing::Test {
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

TEST_F(DualPathKeyframeTest, H264AndRawStreamsWithKeyframeInterval) {
    auto* stub = plugin_.stub();
    constexpr int kKeyframeInterval = 15;
    constexpr int kFrameCount = 30;

    micecam::plugin::OpenStreamRequest h264_req;
    auto* h264_cfg = h264_req.mutable_config();
    h264_cfg->set_device_id("0");
    h264_cfg->set_stream_index(0);
    h264_cfg->set_width(640);
    h264_cfg->set_height(480);
    h264_cfg->set_framerate(30);
    h264_cfg->set_requested_payload(micecam::plugin::PayloadKind::H264);
    h264_cfg->set_keyframe_interval(kKeyframeInterval);
    h264_req.set_ring_slot_count(32);
    h264_req.set_ring_slot_size(65536);

    micecam::plugin::OpenStreamResponse h264_resp;
    grpc::ClientContext h264_ctx;
    h264_ctx.set_deadline(std::chrono::system_clock::now() + kTimeout);
    auto h264_status = stub->OpenStream(&h264_ctx, h264_req, &h264_resp);
    ASSERT_TRUE(h264_status.ok()) << h264_status.error_message();
    ASSERT_TRUE(h264_resp.success()) << h264_resp.error().user_message();
    auto h264_ring_id = h264_resp.ring_descriptor().ring_id();
    auto h264_stream_id = h264_resp.ring_descriptor().stream_id();
    EXPECT_FALSE(h264_ring_id.empty());
    EXPECT_EQ(h264_resp.ring_descriptor().slot_count(), 32u);
    EXPECT_EQ(h264_resp.ring_descriptor().slot_size(), 65536u);
    EXPECT_EQ(h264_resp.ring_descriptor().platform_handle_type(), "posix_shm");
    plugin_.track_shm(h264_ring_id);

    micecam::plugin::OpenStreamRequest raw_req;
    auto* raw_cfg = raw_req.mutable_config();
    raw_cfg->set_device_id("0");
    raw_cfg->set_stream_index(0);
    raw_cfg->set_width(640);
    raw_cfg->set_height(480);
    raw_cfg->set_framerate(30);
    raw_cfg->set_requested_payload(micecam::plugin::PayloadKind::RAW);
    raw_cfg->set_keyframe_interval(kKeyframeInterval);
    raw_req.set_ring_slot_count(8);
    raw_req.set_ring_slot_size(65536);

    micecam::plugin::OpenStreamResponse raw_resp;
    grpc::ClientContext raw_ctx;
    raw_ctx.set_deadline(std::chrono::system_clock::now() + kTimeout);
    auto raw_status = stub->OpenStream(&raw_ctx, raw_req, &raw_resp);
    ASSERT_TRUE(raw_status.ok()) << raw_status.error_message();
    ASSERT_TRUE(raw_resp.success()) << raw_resp.error().user_message();
    auto raw_stream_id = raw_resp.ring_descriptor().stream_id();
    EXPECT_NE(h264_stream_id, raw_stream_id)
        << "Dual-path streams must have distinct IDs";
    plugin_.track_shm(raw_resp.ring_descriptor().ring_id());

    RingAccessor ring;
    ASSERT_TRUE(ring.open(h264_ring_id,
                          h264_resp.ring_descriptor().slot_count(),
                          h264_resp.ring_descriptor().slot_size()));

    std::vector<uint8_t> test_payload(1024, 0xAB);
    uint32_t h264_kind = 2;

    for (int i = 0; i < kFrameCount; i++) {
        bool is_keyframe = (i % kKeyframeInterval == 0);
        ring.write_frame(static_cast<uint64_t>(i), test_payload,
                         h264_kind, is_keyframe);
    }

    for (int i = 0; i < kFrameCount; i++) {
        bool expected = (i % kKeyframeInterval == 0);
        bool actual = ring.read_keyframe_flag(static_cast<uint64_t>(i));
        EXPECT_EQ(actual, expected)
            << "Frame " << i << ": keyframe flag mismatch "
            << "(expected=" << expected << ", actual=" << actual << ")";
    }

    ring.close_ring();

    micecam::plugin::ShutdownRequest shutdown_req;
    micecam::plugin::ShutdownResponse shutdown_resp;
    grpc::ClientContext shutdown_ctx;
    shutdown_ctx.set_deadline(std::chrono::system_clock::now() + kTimeout);
    auto s = stub->Shutdown(&shutdown_ctx, shutdown_req, &shutdown_resp);
    EXPECT_TRUE(s.ok());
}

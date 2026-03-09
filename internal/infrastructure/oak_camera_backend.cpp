#include "infrastructure/oak_camera_backend.h"
#include <depthai/depthai.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace micecam {

class VirtualOAKBackend : public ICameraBackend {
public:
    VirtualOAKBackend(std::shared_ptr<OAKCameraBackend> master, int socket_index)
        : master_(std::move(master)), socket_index_(socket_index) {}

    bool initialize(const CameraConfig& config) override { return true; }
    bool start() override { return true; }
    void stop() override {}

    std::unique_ptr<Frame> get_frame() override;

    uint64_t get_frame_count() const override { return frame_count_; }
    bool is_running() const override { return master_ && master_->is_running(); }
    std::string get_backend_name() const override {
        return "OAK_CAM_" + std::string(1, 'A' + socket_index_);
    }

private:
    std::shared_ptr<OAKCameraBackend> master_;
    int socket_index_;
    uint64_t frame_count_{0};
    friend class OAKCameraBackend;
};

struct OAKCameraBackend::Impl {
    std::shared_ptr<dai::Device> device;
    dai::Pipeline pipeline;
    std::shared_ptr<dai::DataOutputQueue> syncQueue;

    std::atomic<bool> running_{false};
    std::atomic<uint64_t> total_groups_{0};

    std::mutex mtx;
    std::condition_variable cv;
    std::queue<std::shared_ptr<dai::MessageGroup>> proxy_queues[4];
    const size_t MAX_QUEUE_SIZE = 10;

    std::thread distributor_thread;

    void distributor_loop() {
        while (running_) {
            try {
                auto group = syncQueue->get<dai::MessageGroup>();
                if (!group || !running_) break;

                std::unique_lock<std::mutex> lock(mtx);
                total_groups_++;
                for (int i = 0; i < 4; ++i) {
                    if (proxy_queues[i].size() >= MAX_QUEUE_SIZE) {
                        proxy_queues[i].pop();
                    }
                    proxy_queues[i].push(group);
                }
                lock.unlock();
                cv.notify_all();
            } catch (...) {
                break;
            }
        }
    }
};

OAKCameraBackend::OAKCameraBackend() : impl_(std::make_unique<Impl>()) {}

OAKCameraBackend::~OAKCameraBackend() {
    stop();
}

bool OAKCameraBackend::initialize(const CameraConfig& config) {
    try {
        auto boardConfig = dai::BoardConfig();
        boardConfig.gpio[42] = dai::BoardConfig::GPIO(
            dai::BoardConfig::GPIO::Direction::INPUT,
            dai::BoardConfig::GPIO::Level::HIGH,
            dai::BoardConfig::GPIO::Pull::PULL_DOWN
        );
        impl_->pipeline.setBoardConfig(boardConfig);

        auto sync = impl_->pipeline.create<dai::node::Sync>();
        sync->setSyncThreshold(std::chrono::milliseconds(50));

        auto xout = impl_->pipeline.create<dai::node::XLinkOut>();
        xout->setStreamName("quad_sync");
        sync->out.link(xout->input);

        std::vector<std::pair<dai::CameraBoardSocket, std::string>> sockets = {
            {dai::CameraBoardSocket::CAM_A, "CAM_A"},
            {dai::CameraBoardSocket::CAM_B, "CAM_B"},
            {dai::CameraBoardSocket::CAM_C, "CAM_C"},
            {dai::CameraBoardSocket::CAM_D, "CAM_D"}
        };

        for(const auto& [socket, name] : sockets) {
            auto cam = impl_->pipeline.create<dai::node::ColorCamera>();
            cam->setBoardSocket(socket);
            cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
            cam->setFps(config.fps);

            if (socket == dai::CameraBoardSocket::CAM_A) {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
            } else {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
            }

            auto encoder = impl_->pipeline.create<dai::node::VideoEncoder>();
            encoder->setDefaultProfilePreset(config.fps, dai::VideoEncoderProperties::Profile::MJPEG);

            cam->video.link(encoder->input);
            encoder->bitstream.link(sync->inputs[name]);
        }

        int retries = 3;
        while (retries > 0) {
            try {
                impl_->device = std::make_shared<dai::Device>(impl_->pipeline);
                break;
            } catch (const std::exception& e) {
                if (--retries == 0) throw;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }

        impl_->syncQueue = impl_->device->getOutputQueue("quad_sync", 8, false);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "OAK Init Error: " << e.what() << "\n";
        return false;
    }
}

bool OAKCameraBackend::start() {
    if (impl_->running_) return false;
    impl_->running_ = true;
    impl_->distributor_thread = std::thread(&Impl::distributor_loop, impl_.get());
    return true;
}

void OAKCameraBackend::stop() {
    if (!impl_->running_) return;
    impl_->running_ = false;
    if (impl_->syncQueue) impl_->syncQueue->close(); // Wake up get()
    if (impl_->distributor_thread.joinable()) {
        impl_->distributor_thread.join();
    }
    if (impl_->device) impl_->device->close();
}

std::unique_ptr<Frame> OAKCameraBackend::get_frame() {
    return nullptr; // Master doesn't produce frames directly in quad mode
}

uint64_t OAKCameraBackend::get_frame_count() const {
    return impl_->total_groups_.load();
}

bool OAKCameraBackend::is_running() const {
    return impl_->running_.load();
}

std::shared_ptr<OAKCameraBackend> OAKCameraBackend::create_master() {
    return std::make_shared<OAKCameraBackend>();
}

std::unique_ptr<ICameraBackend> OAKCameraBackend::create_proxy(int socket_index) {
    if (socket_index < 0 || socket_index > 3) return nullptr;
    return std::make_unique<VirtualOAKBackend>(shared_from_this(), socket_index);
}

std::unique_ptr<Frame> VirtualOAKBackend::get_frame() {
    if (!master_ || !master_->impl_->running_) return nullptr;
    auto& impl = *master_->impl_;

    std::unique_lock<std::mutex> lock(impl.mtx);
    impl.cv.wait(lock, [&]{ return !impl.running_ || !impl.proxy_queues[socket_index_].empty(); });

    if (!impl.running_ || impl.proxy_queues[socket_index_].empty()) return nullptr;

    auto group = impl.proxy_queues[socket_index_].front();
    impl.proxy_queues[socket_index_].pop();
    lock.unlock();

    std::string name = "CAM_" + std::string(1, 'A' + socket_index_);
    auto imgFrame = group->get<dai::ImgFrame>(name);
    if (!imgFrame) return nullptr;

    frame_count_++;
    auto data = std::make_unique<std::vector<uint8_t>>(imgFrame->getData());
    return std::make_unique<Frame>(frame_count_, std::move(data));
}

} // namespace micecam

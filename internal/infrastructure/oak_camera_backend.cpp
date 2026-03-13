#include "micecam/camera/oak_camera_backend.h"
<<<<<<< HEAD
#include "infrastructure/oak_device_selector.h"

#include <depthai/depthai.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
=======

#include "infrastructure/oak_runtime_session.h"

>>>>>>> feat/windows-packaging
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace micecam {

class VirtualOAKBackend : public ICameraBackend {
public:
    VirtualOAKBackend(std::shared_ptr<OAKCameraBackend> master, int socket_index)
        : master_(std::move(master)), socket_index_(socket_index) {}

    bool initialize(const CameraConfig&) override { return true; }
    bool start() override { return true; }
    void stop() override {}

    std::unique_ptr<Frame> get_frame() override;

    uint64_t get_frame_count() const override { return frame_count_; }
    bool is_running() const override { return master_ && master_->is_running(); }
    std::string get_backend_name() const override {
        return "OAK_CAM_" + std::string(1, static_cast<char>('A' + socket_index_));
    }
    PixelFormat get_current_format() const override { return PixelFormat::MJPEG; }

private:
    std::shared_ptr<OAKCameraBackend> master_;
    int socket_index_;
    uint64_t frame_count_{0};
    friend class OAKCameraBackend;
};

struct OAKCameraBackend::Impl {
<<<<<<< HEAD
    std::shared_ptr<dai::Device> device;
    std::unique_ptr<dai::Pipeline> pipeline;
    std::shared_ptr<dai::MessageQueue> syncQueue;
    std::optional<dai::DeviceInfo> selectedDeviceInfo;

=======
    std::unique_ptr<OAKRuntimeSession> session;
>>>>>>> feat/windows-packaging
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> total_groups_{0};

    std::mutex mtx;
    std::condition_variable cv;
    std::queue<std::shared_ptr<OAKFrameGroup>> proxy_queues[4];
    static constexpr size_t kMaxQueueSize = 10;

    std::thread distributor_thread;

    void distributor_loop() {
        while(running_) {
            auto group = session ? session->get_group() : nullptr;
            if(!group || !running_) {
                break;
            }

            std::unique_lock<std::mutex> lock(mtx);
            total_groups_++;
            for(int i = 0; i < 4; ++i) {
                if(proxy_queues[i].size() >= kMaxQueueSize) {
                    proxy_queues[i].pop();
                }
                proxy_queues[i].push(group);
            }
            lock.unlock();
            cv.notify_all();
        }

        cv.notify_all();
    }
};

OAKCameraBackend::OAKCameraBackend() : impl_(std::make_unique<Impl>()) {}

OAKCameraBackend::~OAKCameraBackend() {
    stop();
}

bool OAKCameraBackend::initialize(const CameraConfig& config) {
<<<<<<< HEAD
    try {
        const auto availableDevices = dai::DeviceBase::getAllAvailableDevices();
        const auto selectedDeviceInfo = resolve_oak_device_info(availableDevices, config.device_id);
        if (!selectedDeviceInfo.has_value()) {
            std::cerr << "OAK Init Error: Invalid OAK device index " << config.device_id
                      << " (found " << availableDevices.size() << " device(s))\n";
            return false;
        }

        impl_->selectedDeviceInfo = *selectedDeviceInfo;
        impl_->device = std::make_shared<dai::Device>(*impl_->selectedDeviceInfo);
        impl_->pipeline = std::make_unique<dai::Pipeline>(impl_->device);

        auto boardConfig = dai::BoardConfig();
        boardConfig.gpio[42] = dai::BoardConfig::GPIO(
            dai::BoardConfig::GPIO::Direction::INPUT,
            dai::BoardConfig::GPIO::Level::HIGH,
            dai::BoardConfig::GPIO::Pull::PULL_DOWN
        );
        impl_->pipeline->setBoardConfig(boardConfig);

        auto sync = impl_->pipeline->create<dai::node::Sync>();
        sync->setSyncThreshold(std::chrono::milliseconds(50));

        std::vector<std::pair<dai::CameraBoardSocket, std::string>> sockets = {
            {dai::CameraBoardSocket::CAM_A, "CAM_A"},
            {dai::CameraBoardSocket::CAM_B, "CAM_B"},
            {dai::CameraBoardSocket::CAM_C, "CAM_C"},
            {dai::CameraBoardSocket::CAM_D, "CAM_D"}
        };

        for(const auto& [socket, name] : sockets) {
            auto cam = impl_->pipeline->create<dai::node::ColorCamera>();
            cam->setBoardSocket(socket);
            cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
            cam->setFps(config.fps);

            if (socket == dai::CameraBoardSocket::CAM_A) {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
            } else {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
            }

            auto encoder = impl_->pipeline->create<dai::node::VideoEncoder>();
            encoder->setDefaultProfilePreset(config.fps, dai::VideoEncoderProperties::Profile::MJPEG);

            cam->video.link(encoder->input);
            encoder->bitstream.link(sync->inputs[name]);
        }

        impl_->syncQueue = sync->out.createOutputQueue(8, false);

        int retries = 3;
        while (retries > 0) {
            try {
                impl_->pipeline->start();
                break;
            } catch (const std::exception& e) {
                if (--retries == 0) throw;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "OAK Init Error: " << e.what() << "\n";
        return false;
=======
    if(impl_->running_ || impl_->session) {
        stop();
        impl_->session.reset();
>>>>>>> feat/windows-packaging
    }

    impl_->session = std::make_unique<OAKRuntimeSession>();
    OAKSessionConfig session_config;
    session_config.width = config.width;
    session_config.height = config.height;
    session_config.fps = config.fps;
    return impl_->session->initialize(session_config);
}

bool OAKCameraBackend::start() {
    if(impl_->running_ || !impl_->session) {
        return false;
    }

    impl_->running_ = true;
    impl_->distributor_thread = std::thread(&Impl::distributor_loop, impl_.get());
    return true;
}

void OAKCameraBackend::stop() {
    if(impl_->running_) {
        impl_->running_ = false;
        if(impl_->session) {
            impl_->session->stop();
        }
        impl_->cv.notify_all();
        if(impl_->distributor_thread.joinable()) {
            impl_->distributor_thread.join();
        }
    } else if(impl_->session) {
        impl_->session->stop();
    }

    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        for(auto& q : impl_->proxy_queues) {
            while(!q.empty()) {
                q.pop();
            }
        }
    }
<<<<<<< HEAD
    if (impl_->pipeline) {
        impl_->pipeline->stop();
    }
=======
>>>>>>> feat/windows-packaging
}

std::unique_ptr<Frame> OAKCameraBackend::get_frame() {
    return nullptr;
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
    if(socket_index < 0 || socket_index > 3) {
        return nullptr;
    }
    return std::make_unique<VirtualOAKBackend>(shared_from_this(), socket_index);
}

std::unique_ptr<Frame> VirtualOAKBackend::get_frame() {
    if(!master_ || !master_->impl_->running_) {
        return nullptr;
    }

    auto& impl = *master_->impl_;
    std::unique_lock<std::mutex> lock(impl.mtx);
    impl.cv.wait(lock, [&] {
        return !impl.running_ || !impl.proxy_queues[socket_index_].empty();
    });

    if(!impl.running_ || impl.proxy_queues[socket_index_].empty()) {
        return nullptr;
    }

    auto group = impl.proxy_queues[socket_index_].front();
    impl.proxy_queues[socket_index_].pop();
    lock.unlock();

    auto encoded = group->frames[socket_index_];
    if(!encoded || !encoded->data) {
        return nullptr;
    }

    frame_count_++;
<<<<<<< HEAD
    auto data_span = imgFrame->getData();
    auto data = std::make_unique<std::vector<uint8_t>>(data_span.begin(), data_span.end());
    auto frame = std::make_unique<Frame>(frame_count_, std::move(data));
    frame->width = imgFrame->getWidth();
    frame->height = imgFrame->getHeight();
    frame->format = PixelFormat::MJPEG; // OAK default in this impl
    return frame;
=======
    auto frame_data = std::make_unique<std::vector<uint8_t>>(*encoded->data);
    return std::make_unique<Frame>(encoded->sequence_id, std::move(frame_data));
>>>>>>> feat/windows-packaging
}

}  // namespace micecam

#include "micecam/camera/oak_camera_backend.h"
#include <depthai/depthai.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace micecam {

/**
 * @brief Proxy that represents one camera stream from the OAK-4P Master device
 */
class VirtualOAKBackend : public ICameraBackend {
public:
    VirtualOAKBackend(std::shared_ptr<OAKCameraBackend> master, int socket_index)
        : master_(std::move(master)), socket_index_(socket_index) {}

    bool initialize(const CameraConfig& config) override {
        config_ = config;
        return true; 
    }

    bool start() override { return true; }
    void stop() override { }

    std::unique_ptr<Frame> get_frame() override;

    uint64_t get_frame_count() const override { return frame_count_; }
    bool is_running() const override { return master_ && master_->is_running(); }
    std::string get_backend_name() const override {
        return "OAK_CAM_" + std::string(1, 'A' + socket_index_);
    }

private:
    std::shared_ptr<OAKCameraBackend> master_;
    int socket_index_;
    CameraConfig config_;
    uint64_t frame_count_{0};
    friend class OAKCameraBackend;
};

class OAKCameraBackend::Impl {
public:
    std::shared_ptr<dai::Device> device;
    dai::Pipeline pipeline;
    std::shared_ptr<dai::DataOutputQueue> syncQueue;
    
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> total_frame_groups_{0};
    
    // Multi-stream synchronization and distribution
    std::mutex mtx;
    std::condition_variable cv;
    std::shared_ptr<dai::MessageGroup> current_group;
    
    // Tracks which proxies have already consumed the current group
    std::map<int, uint64_t> last_delivered_seq;
};

OAKCameraBackend::OAKCameraBackend() : impl_(std::make_unique<Impl>()) {}

OAKCameraBackend::~OAKCameraBackend() {
    stop();
}

bool OAKCameraBackend::initialize(const CameraConfig& config) {
    try {
        // Build 4-camera sync pipeline
        std::vector<dai::CameraBoardSocket> sockets = {
            dai::CameraBoardSocket::CAM_A,
            dai::CameraBoardSocket::CAM_B,
            dai::CameraBoardSocket::CAM_C,
            dai::CameraBoardSocket::CAM_D
        };

        auto sync = impl_->pipeline.create<dai::node::Sync>();

        for(int i = 0; i < (int)sockets.size(); ++i) {
            auto socket = sockets[i];
            auto cam = impl_->pipeline.create<dai::node::Camera>();
            cam->setBoardSocket(socket);
            
            // Note: B036801 (IMX296) supports 1920x1200 or 1440x1080
            // We use VideoEncoder for device-side MJPEG to keep host CPU low
            auto encoder = impl_->pipeline.create<dai::node::VideoEncoder>();
            encoder->setDefaultProfilePreset(config.fps, dai::VideoEncoderProperties::Profile::MJPEG);
            
            cam->video.link(encoder->input);
            encoder->bitstream.link(sync->inputs[std::to_string(i)]);
        }

        auto xout = impl_->pipeline.create<dai::node::XLinkOut>();
        xout->setStreamName("quad_sync");
        sync->out.link(xout->input);

        // Connect to device
        impl_->device = std::make_shared<dai::Device>(impl_->pipeline);
        impl_->syncQueue = impl_->device->getOutputQueue("quad_sync", 4, false);

        std::cout << "OAK-4P Quad-Camera Backend initialized (HW Sync + MJPEG)\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "OAK Initialization Failed: " << e.what() << "\n";
        return false;
    }
}

bool OAKCameraBackend::start() {
    if (impl_->running_) return false;
    impl_->running_ = true;
    std::cout << "OAK Master capture started\n";
    return true;
}

void OAKCameraBackend::stop() {
    if (!impl_->running_) return;
    impl_->running_ = false;
    impl_->cv.notify_all(); // Wake up any waiting proxies
    if (impl_->device) {
        impl_->device->close();
    }
    std::cout << "OAK Master capture stopped\n";
}

std::unique_ptr<Frame> OAKCameraBackend::get_frame() {
    // Default implementation returns CAM_A (index 0) for backward compatibility
    if (!impl_->syncQueue || !impl_->running_) return nullptr;
    
    std::unique_lock<std::mutex> lock(impl_->mtx);
    // If we haven't pulled the group for this sequence yet, pull it
    if (!impl_->current_group || impl_->last_delivered_seq[0] >= impl_->total_frame_groups_) {
        lock.unlock();
        auto group = impl_->syncQueue->get<dai::MessageGroup>();
        lock.lock();
        
        if (!group || !impl_->running_) return nullptr;
        
        impl_->current_group = group;
        impl_->total_frame_groups_.fetch_add(1);
        impl_->cv.notify_all();
    }
    
    auto imgFrame = impl_->current_group->get<dai::ImgFrame>("0");
    if (!imgFrame) return nullptr;

    auto data = std::make_unique<std::vector<uint8_t>>(imgFrame->getData());
    impl_->last_delivered_seq[0] = impl_->total_frame_groups_;
    
    return std::make_unique<Frame>(impl_->total_frame_groups_, std::move(data));
}

uint64_t OAKCameraBackend::get_frame_count() const {
    return impl_->total_frame_groups_.load();
}

bool OAKCameraBackend::is_running() const {
    return impl_->running_.load();
}

std::shared_ptr<OAKCameraBackend> OAKCameraBackend::create_master() {
    return std::make_shared<OAKCameraBackend>();
}

std::unique_ptr<ICameraBackend> OAKCameraBackend::create_proxy(int socket_index) {
    return std::make_unique<VirtualOAKBackend>(shared_from_this(), socket_index);
}

// Proxy implementation: Wait for the master to fetch the synchronized group
std::unique_ptr<Frame> VirtualOAKBackend::get_frame() {
    if (!master_) return nullptr;
    auto master_ptr = std::dynamic_pointer_cast<OAKCameraBackend>(master_);
    if (!master_ptr || !master_ptr->impl_->syncQueue) return nullptr;
    auto& impl = *master_ptr->impl_;

    std::unique_lock<std::mutex> lock(impl.mtx);
    
    // If I'm the first one asking, I pull the group.
    // If others are asking, they wait for the notification.
    if (!impl.current_group || frame_count_ >= impl.total_frame_groups_) {
        // If no one else is currently pulling, I'll do it
        // (Simple optimization: first proxy to reach this point triggers the fetch)
        if (impl.last_delivered_seq.empty() || 
            std::all_of(impl.last_delivered_seq.begin(), impl.last_delivered_seq.end(), 
                        [&](const auto& pair){ return pair.second >= impl.total_frame_groups_; })) {
            lock.unlock();
            auto group = impl.syncQueue->get<dai::MessageGroup>();
            lock.lock();
            
            if (!group || !impl.running_) return nullptr;
            
            impl.current_group = group;
            impl.total_frame_groups_.fetch_add(1);
            impl.cv.notify_all();
        } else {
            // Wait for another proxy or the master to pull the group
            impl.cv.wait(lock, [&]{ 
                return !impl.running_ || (impl.current_group && frame_count_ < impl.total_frame_groups_); 
            });
        }
    }

    if (!impl.running_ || !impl.current_group) return nullptr;

    auto imgFrame = impl.current_group->get<dai::ImgFrame>(std::to_string(socket_index_));
    if (!imgFrame) return nullptr;

    frame_count_++;
    auto data = std::make_unique<std::vector<uint8_t>>(imgFrame->getData());
    impl.last_delivered_seq[socket_index_] = frame_count_;
    
    return std::make_unique<Frame>(frame_count_, std::move(data));
}

}  // namespace micecam

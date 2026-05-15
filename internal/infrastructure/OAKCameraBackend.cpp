#include "OAKCameraBackend.h"

#include "domain/Capabilities.h"

namespace micecam::infrastructure {

#ifdef WITH_DEPTHAI
#include <depthai/depthai.hpp>

struct OAKCameraBackend::Impl {
    std::shared_ptr<dai::Device> device;
};

class OAKCameraStream : public domain::CameraStream {
public:
    OAKCameraStream(std::shared_ptr<dai::DataOutputQueue> q, int w, int h, int fps)
        : queue_(std::move(q)), width_(w), height_(h), fps_(fps) {}

    bool read_frame(std::vector<uint8_t>& out_data, int64_t& out_pts) override {
        auto msg = queue_->get<dai::ImgFrame>();
        if (!msg) return false;
        auto& data = msg->getData();
        out_data.assign(data.begin(), data.end());
        auto ts = msg->getTimestamp();
        out_pts = static_cast<int64_t>(ts.time_since_epoch().count());
        return true;
    }
    int width() const override { return width_; }
    int height() const override { return height_; }
    int fps() const override { return fps_; }
    std::string pixel_format() const override { return "nv12"; }
    bool is_open() const override { return queue_ != nullptr; }
    void close() override { queue_.reset(); }

private:
    std::shared_ptr<dai::DataOutputQueue> queue_;
    int width_, height_, fps_;
};

std::vector<domain::DeviceInfo> OAKCameraBackend::enumerate_devices() {
    std::vector<domain::DeviceInfo> result;
    try {
        auto devices = dai::Device::getAllAvailableDevices();
        for (auto& d : devices) {
            domain::DeviceInfo info;
            info.id = d.getMxId();
            info.name = d.name;
            info.type = "oak";
            info.serial = d.getMxId();
            for (int i = 0; i < 4; i++) {
                domain::StreamInfo si;
                si.index = i;
                si.max_width = 4096;
                si.max_height = 2160;
                si.label = std::string("CAM_") + static_cast<char>('A' + i);
                si.resolutions = {
                    {4096, 2160, "4K"},
                    {1920, 1080, "1080p"},
                };
                si.supported_formats = {"nv12"};
                si.supported_framerates = {30, 60};
                si.available = true;
                info.streams.push_back(si);
            }
            result.push_back(info);
        }
    } catch (...) {}
    return result;
}

std::unique_ptr<domain::CameraStream> OAKCameraBackend::open_stream(const domain::StreamConfig& config) {
    try {
        auto devices = dai::Device::getAllAvailableDevices();
        if (devices.empty()) return nullptr;

        auto device = std::make_shared<dai::Device>(dai::Pipeline(), devices[0]);
        if (!device) return nullptr;

        auto pipeline = std::make_shared<dai::Pipeline>();
        auto cam = pipeline->create<dai::node::ColorCamera>();
        cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
        cam->setFps(30);

        auto enc = pipeline->create<dai::node::VideoEncoder>();
        enc->setDefaultProfilePreset(30, dai::VideoEncoderProperties::Profile::H264_MAIN);
        enc->setBitrateKbps(5000);
        cam->video.link(enc->input);

        auto xout = pipeline->create<dai::node::XLinkOut>();
        xout->setStreamName("h264");
        enc->bitstream.link(xout->input);

        device->startPipeline(*pipeline);
        auto queue = device->getOutputQueue("h264", 8, false);

        return std::make_unique<OAKCameraStream>(queue,
            config.width > 0 ? config.width : 1920,
            config.height > 0 ? config.height : 1080,
            config.framerate > 0 ? config.framerate : 30);
    } catch (...) {
        return nullptr;
    }
}

domain::Capabilities OAKCameraBackend::get_capabilities() {
    domain::Capabilities caps;
    caps.supports_hardware_encode = true;
    caps.encoder_name = "h264_oak";
    domain::StreamInfo si;
    si.index = 0;
    si.max_width = 4096;
    si.max_height = 2160;
    si.label = "CAM_A";
    si.resolutions = {
        {4096, 2160, "4K"},
        {1920, 1080, "1080p"},
    };
    si.supported_formats = {"nv12"};
    si.supported_framerates = {30, 60};
    si.available = true;
    caps.streams.push_back(si);
    return caps;
}

#else // !WITH_DEPTHAI

std::vector<domain::DeviceInfo> OAKCameraBackend::enumerate_devices() {
    return {};
}

std::unique_ptr<domain::CameraStream> OAKCameraBackend::open_stream(const domain::StreamConfig&) {
    return nullptr;
}

domain::Capabilities OAKCameraBackend::get_capabilities() {
    domain::Capabilities caps;
    caps.supports_hardware_encode = true;
    caps.encoder_name = "h264_oak";
    domain::StreamInfo si;
    si.index = 0;
    si.max_width = 4096;
    si.max_height = 2160;
    si.label = "CAM_A";
    si.resolutions = {
        {4096, 2160, "4K"},
        {1920, 1080, "1080p"},
    };
    si.supported_formats = {"nv12"};
    si.supported_framerates = {30, 60};
    si.available = true;
    caps.streams.push_back(si);
    return caps;
}

#endif // WITH_DEPTHAI

} // namespace micecam::infrastructure

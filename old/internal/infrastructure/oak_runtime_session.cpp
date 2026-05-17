#include "infrastructure/oak_runtime_session.h"

#include <depthai/depthai.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace micecam {
namespace {

constexpr std::array<dai::CameraBoardSocket, 4> kSockets = {
    dai::CameraBoardSocket::CAM_A,
    dai::CameraBoardSocket::CAM_B,
    dai::CameraBoardSocket::CAM_C,
    dai::CameraBoardSocket::CAM_D,
};

constexpr std::array<const char*, 4> kStreamNames = {
    "CAM_A",
    "CAM_B",
    "CAM_C",
    "CAM_D",
};

bool contains_type(const std::vector<dai::CameraSensorType>& supported_types, dai::CameraSensorType type) {
    for(const auto supported_type : supported_types) {
        if(supported_type == type) {
            return true;
        }
    }
    return false;
}

bool is_mono_sensor(const dai::CameraFeatures& feature) {
    if(feature.sensorName == "OV9282") return true;
    if(feature.sensorName == "OV9782") return false;
    if(contains_type(feature.supportedTypes, dai::CameraSensorType::MONO)
       && !contains_type(feature.supportedTypes, dai::CameraSensorType::COLOR)) {
        return true;
    }
    return false;
}

dai::ColorCameraProperties::SensorResolution choose_color_resolution(int height) {
    if(height >= 1200) return dai::ColorCameraProperties::SensorResolution::THE_1200_P;
    if(height >= 1080) return dai::ColorCameraProperties::SensorResolution::THE_1080_P;
    if(height >= 800) return dai::ColorCameraProperties::SensorResolution::THE_800_P;
    return dai::ColorCameraProperties::SensorResolution::THE_720_P;
}

dai::MonoCameraProperties::SensorResolution choose_mono_resolution(int height) {
    if(height >= 800) return dai::MonoCameraProperties::SensorResolution::THE_800_P;
    if(height >= 720) return dai::MonoCameraProperties::SensorResolution::THE_720_P;
    return dai::MonoCameraProperties::SensorResolution::THE_400_P;
}

}  // namespace

class OAKRuntimeSession::Impl {
public:
    std::shared_ptr<dai::Device> device;
    std::unique_ptr<dai::Pipeline> pipeline;
    std::shared_ptr<dai::MessageQueue> sync_queue;
    bool initialized = false;
};

OAKRuntimeSession::OAKRuntimeSession() : impl_(std::make_unique<Impl>()) {}

OAKRuntimeSession::~OAKRuntimeSession() {
    stop();
}

bool OAKRuntimeSession::initialize(const OAKSessionConfig& config) {
    stop();

    try {
        const auto available_devices = dai::Device::getAllAvailableDevices();
        if(available_devices.empty()) {
            std::cerr << "OAK Init Error: no available OAK devices\n";
            return false;
        }

        impl_->device = std::make_shared<dai::Device>(available_devices.front());
        const auto camera_features = impl_->device->getConnectedCameraFeatures();
        if(camera_features.empty()) {
            std::cerr << "OAK Init Error: no connected OAK cameras detected\n";
            return false;
        }

        std::unordered_map<dai::CameraBoardSocket, dai::CameraFeatures> feature_by_socket;
        for(const auto& feature : camera_features) {
            feature_by_socket.emplace(feature.socket, feature);
        }

        impl_->pipeline = std::make_unique<dai::Pipeline>(impl_->device);
        impl_->pipeline->setXLinkChunkSize(0);

        if(feature_by_socket.count(dai::CameraBoardSocket::CAM_A) != 0
           && feature_by_socket.at(dai::CameraBoardSocket::CAM_A).sensorName == "OV9782") {
            auto board_config = dai::BoardConfig();
            board_config.gpio[42] = dai::BoardConfig::GPIO(
                dai::BoardConfig::GPIO::Direction::INPUT,
                dai::BoardConfig::GPIO::Level::HIGH,
                dai::BoardConfig::GPIO::Pull::PULL_DOWN);
            impl_->pipeline->setBoardConfig(board_config);
        }

        auto sync = impl_->pipeline->create<dai::node::Sync>();
        sync->setSyncThreshold(std::chrono::milliseconds(50));

        for(size_t i = 0; i < kSockets.size(); ++i) {
            const auto socket = kSockets[i];
            const auto name = std::string{kStreamNames[i]};

            if(feature_by_socket.count(socket) == 0) {
                continue;
            }

            const auto& feature = feature_by_socket.at(socket);
            const bool mono = is_mono_sensor(feature);

            auto encoder = impl_->pipeline->create<dai::node::VideoEncoder>();
            encoder->setDefaultProfilePreset(static_cast<float>(config.fps), dai::VideoEncoderProperties::Profile::MJPEG);
            encoder->bitstream.link(sync->inputs[name]);

            if(mono) {
                auto cam = impl_->pipeline->create<dai::node::MonoCamera>();
                cam->setBoardSocket(socket);
                cam->setResolution(choose_mono_resolution(config.height));
                cam->setFps(static_cast<float>(config.fps));
                cam->out.link(encoder->input);

                if(socket == dai::CameraBoardSocket::CAM_A) {
                    cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
                } else {
                    cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
                }
            } else {
                auto cam = impl_->pipeline->create<dai::node::ColorCamera>();
                cam->setBoardSocket(socket);
                cam->setResolution(choose_color_resolution(config.height));
                cam->setFps(static_cast<float>(config.fps));
                cam->video.link(encoder->input);

                if(socket == dai::CameraBoardSocket::CAM_A) {
                    cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
                } else {
                    cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
                }
            }
        }

        impl_->sync_queue = sync->out.createOutputQueue(4, false);
        impl_->pipeline->start();
        impl_->initialized = true;
        return true;
    } catch(const std::exception& e) {
        std::cerr << "OAK Init Error: " << e.what() << "\n";
        stop();
        return false;
    }
}

void OAKRuntimeSession::stop() {
    if(impl_->sync_queue) {
        impl_->sync_queue->close();
        impl_->sync_queue.reset();
    }
    if(impl_->pipeline) {
        impl_->pipeline->stop();
    }
    if(impl_->device && !impl_->device->isClosed()) {
        impl_->device->close();
    }
    impl_->pipeline.reset();
    impl_->device.reset();
    impl_->initialized = false;
}

std::shared_ptr<OAKFrameGroup> OAKRuntimeSession::get_group() {
    if(!impl_->initialized || !impl_->sync_queue) {
        return nullptr;
    }

    try {
        auto msg = impl_->sync_queue->get<dai::MessageGroup>();
        if(!msg) {
            return nullptr;
        }

        auto group = std::make_shared<OAKFrameGroup>();
        for(size_t i = 0; i < kStreamNames.size(); ++i) {
            auto encoded = msg->get<dai::EncodedFrame>(kStreamNames[i]);
            if(!encoded) {
                return nullptr;
            }

            auto data_span = encoded->getData();
            auto frame = std::make_shared<OAKEncodedFrame>();
            frame->sequence_id = encoded->getSequenceNum();
            frame->width = encoded->getWidth();
            frame->height = encoded->getHeight();
            frame->data = std::make_shared<std::vector<uint8_t>>(data_span.begin(), data_span.end());
            group->frames[i] = std::move(frame);
        }

        return group;
    } catch(...) {
        return nullptr;
    }
}

}  // namespace micecam

#include <depthai/depthai.hpp>

#include "infrastructure/oak_runtime_session.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

namespace micecam {

class OAKRuntimeSession::Impl {
public:
    std::unique_ptr<dai::Pipeline> pipeline;
    std::shared_ptr<dai::MessageQueue> sync_queue;
    bool initialized = false;
};

OAKRuntimeSession::OAKRuntimeSession() : impl_(std::make_unique<Impl>()) {}

OAKRuntimeSession::~OAKRuntimeSession() {
    stop();
}

bool OAKRuntimeSession::initialize(const OAKSessionConfig& config) {
    try {
        impl_->pipeline = std::make_unique<dai::Pipeline>();

        auto board_config = dai::BoardConfig();
        board_config.gpio[42] = dai::BoardConfig::GPIO(
            dai::BoardConfig::GPIO::Direction::INPUT,
            dai::BoardConfig::GPIO::Level::HIGH,
            dai::BoardConfig::GPIO::Pull::PULL_DOWN
        );
        impl_->pipeline->setBoardConfig(board_config);

        auto sync = impl_->pipeline->create<dai::node::Sync>();
        sync->setSyncThreshold(std::chrono::milliseconds(50));

        const std::vector<std::pair<dai::CameraBoardSocket, std::string>> sockets = {
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

            if(socket == dai::CameraBoardSocket::CAM_A) {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
            } else {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
            }

            auto encoder = impl_->pipeline->create<dai::node::VideoEncoder>();
            encoder->setDefaultProfilePreset(static_cast<int>(config.fps), dai::VideoEncoderProperties::Profile::MJPEG);

            cam->video.link(encoder->input);
            encoder->bitstream.link(sync->inputs[name]);
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
        impl_->pipeline.reset();
    }
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

        static const std::array<const char*, 4> names = {"CAM_A", "CAM_B", "CAM_C", "CAM_D"};
        auto group = std::make_shared<OAKFrameGroup>();

        for(size_t i = 0; i < names.size(); ++i) {
            auto img = msg->get<dai::ImgFrame>(names[i]);
            if(!img) {
                return nullptr;
            }

            auto data_span = img->getData();
            auto frame = std::make_shared<OAKEncodedFrame>();
            frame->sequence_id = img->getSequenceNum();
            frame->width = img->getWidth();
            frame->height = img->getHeight();
            frame->data = std::make_shared<std::vector<uint8_t>>(data_span.begin(), data_span.end());
            group->frames[i] = std::move(frame);
        }

        return group;
    } catch(...) {
        return nullptr;
    }
}

}  // namespace micecam

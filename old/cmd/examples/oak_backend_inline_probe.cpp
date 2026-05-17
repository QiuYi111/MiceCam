#include <depthai/depthai.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    try {
        auto pipeline = std::make_unique<dai::Pipeline>();

        auto boardConfig = dai::BoardConfig();
        boardConfig.gpio[42] = dai::BoardConfig::GPIO(
            dai::BoardConfig::GPIO::Direction::INPUT,
            dai::BoardConfig::GPIO::Level::HIGH,
            dai::BoardConfig::GPIO::Pull::PULL_DOWN
        );
        pipeline->setBoardConfig(boardConfig);

        auto sync = pipeline->create<dai::node::Sync>();
        sync->setSyncThreshold(std::chrono::milliseconds(50));

        std::vector<std::pair<dai::CameraBoardSocket, std::string>> sockets = {
            {dai::CameraBoardSocket::CAM_A, "CAM_A"},
            {dai::CameraBoardSocket::CAM_B, "CAM_B"},
            {dai::CameraBoardSocket::CAM_C, "CAM_C"},
            {dai::CameraBoardSocket::CAM_D, "CAM_D"}
        };

        for(const auto& [socket, name] : sockets) {
            auto cam = pipeline->create<dai::node::ColorCamera>();
            cam->setBoardSocket(socket);
            cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
            cam->setFps(30);

            if(socket == dai::CameraBoardSocket::CAM_A) {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
            } else {
                cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
            }

            auto encoder = pipeline->create<dai::node::VideoEncoder>();
            encoder->setDefaultProfilePreset(30, dai::VideoEncoderProperties::Profile::MJPEG);

            cam->video.link(encoder->input);
            encoder->bitstream.link(sync->inputs[name]);
        }

        auto q = sync->out.createOutputQueue(4, false);
        pipeline->start();

        auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
            auto msg = q->tryGet<dai::MessageGroup>();
            if(msg) {
                std::cout << "oak_backend_inline_probe: success size=" << msg->getNumMessages() << "\n";
                return 0;
            }
        }

        std::cerr << "oak_backend_inline_probe: timeout\n";
        return 1;
    } catch(const std::exception& e) {
        std::cerr << "oak_backend_inline_probe exception: " << e.what() << "\n";
        return 1;
    }
}

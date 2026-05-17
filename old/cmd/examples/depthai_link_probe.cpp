#include <depthai/depthai.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#ifdef _MSC_VER
#include <crtdbg.h>

namespace {
void invalid_parameter_handler(const wchar_t* expression,
                               const wchar_t* function,
                               const wchar_t* file,
                               unsigned int line,
                               uintptr_t) {
    std::wcerr << L"CRT invalid parameter";
    if(function) {
        std::wcerr << L" function=" << function;
    }
    if(expression) {
        std::wcerr << L" expression=" << expression;
    }
    if(file) {
        std::wcerr << L" file=" << file << L":" << line;
    }
    std::wcerr << std::endl;
}
}  // namespace
#endif

int main(int argc, char** argv) {
#ifdef _MSC_VER
    _set_invalid_parameter_handler(invalid_parameter_handler);
    _CrtSetReportMode(_CRT_ASSERT, 0);
    _CrtSetReportMode(_CRT_ERROR, 0);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    const std::string mode = argc > 1 ? argv[1] : "main";
    std::cout << "depthai_link_probe: mode=" << mode << std::endl;

    if(mode == "available") {
        const auto devices = dai::Device::getAllAvailableDevices();
        std::cout << "available_count=" << devices.size() << std::endl;
        return 0;
    }

    if(mode == "device") {
        const auto devices = dai::Device::getAllAvailableDevices();
        std::cout << "available_count=" << devices.size() << std::endl;
        if(devices.empty()) return 1;
        auto device = std::make_shared<dai::Device>(devices.front());
        std::cout << "device_opened" << std::endl;
        return 0;
    }

    if(mode == "features") {
        const auto devices = dai::Device::getAllAvailableDevices();
        std::cout << "available_count=" << devices.size() << std::endl;
        if(devices.empty()) return 1;
        auto device = std::make_shared<dai::Device>(devices.front());
        std::cout << "device_opened" << std::endl;
        const auto features = device->getConnectedCameraFeatures();
        std::cout << "feature_count=" << features.size() << std::endl;
        for(const auto& feature : features) {
            std::cout << "socket=" << dai::toString(feature.socket)
                      << " sensor=" << feature.sensorName
                      << " types=" << feature.supportedTypes.size()
                      << std::endl;
        }
        return 0;
    }

    if(mode == "pipeline") {
        const auto devices = dai::Device::getAllAvailableDevices();
        std::cout << "available_count=" << devices.size() << std::endl;
        if(devices.empty()) return 1;
        auto device = std::make_shared<dai::Device>(devices.front());
        std::cout << "device_opened" << std::endl;
        auto pipeline = std::make_unique<dai::Pipeline>(device);
        std::cout << "pipeline_created" << std::endl;

        auto sync = pipeline->create<dai::node::Sync>();
        sync->setSyncThreshold(std::chrono::milliseconds(50));
        std::cout << "sync_created" << std::endl;

        auto features = device->getConnectedCameraFeatures();
        std::unordered_map<dai::CameraBoardSocket, dai::CameraFeatures> feature_by_socket;
        for(const auto& feature : features) {
            feature_by_socket.emplace(feature.socket, feature);
        }

        const std::vector<std::pair<dai::CameraBoardSocket, std::string>> sockets = {
            {dai::CameraBoardSocket::CAM_A, "CAM_A"},
            {dai::CameraBoardSocket::CAM_B, "CAM_B"},
            {dai::CameraBoardSocket::CAM_C, "CAM_C"},
            {dai::CameraBoardSocket::CAM_D, "CAM_D"},
        };

        for(const auto& [socket, name] : sockets) {
            if(feature_by_socket.count(socket) == 0) continue;

            const auto sensor = feature_by_socket.at(socket).sensorName;
            const bool mono = sensor == "OV9282";
            std::cout << "building_socket=" << name << " sensor=" << sensor << " mono=" << mono << std::endl;

            auto encoder = pipeline->create<dai::node::VideoEncoder>();
            encoder->setDefaultProfilePreset(30.0f, dai::VideoEncoderProperties::Profile::MJPEG);
            encoder->bitstream.link(sync->inputs[name]);

            if(mono) {
                auto cam = pipeline->create<dai::node::MonoCamera>();
                cam->setBoardSocket(socket);
                cam->setResolution(dai::MonoCameraProperties::SensorResolution::THE_800_P);
                cam->setFps(30.0f);
                cam->out.link(encoder->input);
            } else {
                auto cam = pipeline->create<dai::node::ColorCamera>();
                cam->setBoardSocket(socket);
                cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
                cam->setFps(30.0f);
                cam->video.link(encoder->input);
            }
        }

        auto q = sync->out.createOutputQueue(4, false);
        std::cout << "queue_created" << std::endl;
        pipeline->start();
        std::cout << "pipeline_started" << std::endl;
        auto msg = q->get<dai::MessageGroup>();
        std::cout << "message_group=" << (msg ? msg->getNumMessages() : -1) << std::endl;
        pipeline->stop();
        return 0;
    }

    return 0;
}

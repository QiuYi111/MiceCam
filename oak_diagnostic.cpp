#include <depthai/depthai.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

// Helper to test a single socket
bool test_socket(dai::CameraBoardSocket socket, std::string socket_name) {
    std::cout << "\n------------------------------------------------\n";
    std::cout << "DIAGNOSTIC: Testing " << socket_name << " (OV9782 800P)\n";
    
    dai::Pipeline pipeline;
    auto cam = pipeline.create<dai::node::ColorCamera>();
    auto xout = pipeline.create<dai::node::XLinkOut>();
    
    cam->setBoardSocket(socket);
    cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
    cam->setFps(30);
    
    // IMPORTANT: Set to AUTO first to ensure it generates frames without waiting for triggering
    cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OFF); 
    
    xout->setStreamName("preview");
    cam->isp.link(xout->input);
    
    try {
        dai::Device device(pipeline);
        auto q = device.getOutputQueue("preview", 8, false);
        
        std::cout << "Waiting for frames (timeout 3s)...\n";
        bool received = false;
        
        auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
            auto frame = q->tryGet<dai::ImgFrame>();
            if(frame) {
                std::cout << "SUCCESS: " << socket_name << " produced frame " << frame->getSequenceNum() 
                          << " (" << frame->getWidth() << "x" << frame->getHeight() << ")\n";
                received = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if(!received) {
            std::cout << "FAILURE: " << socket_name << " timed out (No frames)\n";
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        return false;
    }
    return true;
}

// Helper to test sync between two cameras
bool test_sync_pair(dai::CameraBoardSocket master, dai::CameraBoardSocket slave) {
    std::cout << "\n------------------------------------------------\n";
    std::cout << "DIAGNOSTIC: Testing HW SYNC (Master: A, Slave: B)\n";
    
    dai::Pipeline pipeline;
    
    // Master
    auto camA = pipeline.create<dai::node::ColorCamera>();
    camA->setBoardSocket(master);
    camA->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
    camA->setFps(30);
    camA->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
    
    // Slave
    auto camB = pipeline.create<dai::node::ColorCamera>();
    camB->setBoardSocket(slave);
    camB->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
    camB->setFps(30);
    camB->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
    
    // Board Config (OV9782 specific from ROS)
    auto boardConfig = dai::BoardConfig();
    boardConfig.gpio[42] = dai::BoardConfig::GPIO(
        dai::BoardConfig::GPIO::Direction::INPUT, 
        dai::BoardConfig::GPIO::Level::HIGH, 
        dai::BoardConfig::GPIO::Pull::PULL_DOWN
    );
    pipeline.setBoardConfig(boardConfig);

    auto xoutA = pipeline.create<dai::node::XLinkOut>();
    xoutA->setStreamName("previewA");
    camA->isp.link(xoutA->input);
    
    auto xoutB = pipeline.create<dai::node::XLinkOut>();
    xoutB->setStreamName("previewB");
    camB->isp.link(xoutB->input);
    
    try {
        dai::Device device(pipeline);
        auto qA = device.getOutputQueue("previewA", 4, false);
        auto qB = device.getOutputQueue("previewB", 4, false);
        
        std::cout << "Waiting for synchronized frames (timeout 3s)...\n";
        bool received = false;
        
        auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
            auto frameA = qA->tryGet<dai::ImgFrame>();
            auto frameB = qB->tryGet<dai::ImgFrame>();
            
            if(frameA && frameB) {
                std::cout << "SUCCESS: Sync Pair produced frames!\n";
                std::cout << "  Frame A: " << frameA->getSequenceNum() << " Ts: " << frameA->getTimestamp().time_since_epoch().count() << "\n";
                std::cout << "  Frame B: " << frameB->getSequenceNum() << " Ts: " << frameB->getTimestamp().time_since_epoch().count() << "\n";
                
                // Check timestamp diff
                auto diff = std::chrono::duration_cast<std::chrono::microseconds>(frameA->getTimestamp() - frameB->getTimestamp()).count();
                std::cout << "  Diff: " << diff << " us\n";
                
                if (std::abs(diff) < 1000) {
                     std::cout << "  Sync Status: EXCELLENT (<1ms)\n";
                } else {
                     std::cout << "  Sync Status: POOR (>1ms)\n";
                }
                received = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if(!received) {
            std::cout << "FAILURE: Sync Pair timeout (Slave didn't trigger?)\n";
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        return false;
    }
    return true;
}

// Helper to test full quad sync using Sync node AND VideoEncoder (Backend Logic)
bool test_quad_sync() {
    std::cout << "\n------------------------------------------------\n";
    std::cout << "DIAGNOSTIC: Testing QUAD SYNC (Sync Node + VideoEncoder)\n";
    
    dai::Pipeline pipeline;
    auto sync = pipeline.create<dai::node::Sync>();
    sync->setSyncThreshold(std::chrono::milliseconds(50));
    
    auto xout = pipeline.create<dai::node::XLinkOut>();
    xout->setStreamName("quad_sync");
    sync->out.link(xout->input);
    
    std::vector<dai::CameraBoardSocket> sockets = {
        dai::CameraBoardSocket::CAM_A, dai::CameraBoardSocket::CAM_B,
        dai::CameraBoardSocket::CAM_C, dai::CameraBoardSocket::CAM_D
    };
    std::vector<std::string> names = {"CAM_A", "CAM_B", "CAM_C", "CAM_D"};
    
    // Board Config (OV9782 specific from ROS)
    auto boardConfig = dai::BoardConfig();
    boardConfig.gpio[42] = dai::BoardConfig::GPIO(
        dai::BoardConfig::GPIO::Direction::INPUT, 
        dai::BoardConfig::GPIO::Level::HIGH, 
        dai::BoardConfig::GPIO::Pull::PULL_DOWN
    );
    pipeline.setBoardConfig(boardConfig);

    for(size_t i=0; i<sockets.size(); ++i) {
        auto cam = pipeline.create<dai::node::ColorCamera>();
        cam->setBoardSocket(sockets[i]);
        cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
        cam->setFps(30);
        
        if (sockets[i] == dai::CameraBoardSocket::CAM_A) {
            cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::OUTPUT);
        } else {
            cam->initialControl.setFrameSyncMode(dai::CameraControl::FrameSyncMode::INPUT);
        }
        
        // Add VideoEncoder (MJPEG)
        auto encoder = pipeline.create<dai::node::VideoEncoder>();
        encoder->setDefaultProfilePreset(30, dai::VideoEncoderProperties::Profile::MJPEG);
        
        cam->video.link(encoder->input);
        encoder->bitstream.link(sync->inputs[names[i]]);
    }
    
    try {
        dai::Device device(pipeline);
        auto q = device.getOutputQueue("quad_sync", 4, false);
        
        std::cout << "Waiting for quad-synchronized MJPEG frames (timeout 3s)...\n";
        bool received = false;
        
        auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
            auto msg = q->tryGet<dai::MessageGroup>();
            if(msg) {
                std::cout << "SUCCESS: Quad Sync (MJPEG) produced MessageGroup! Size: " << msg->getNumMessages() << "\n";
                // Print timestamps (ImgFrame from encoder is the bitstream)
                for(const auto& name : names) {
                    if(auto img = msg->get<dai::ImgFrame>(name)) {
                         std::cout << "  " << name << " Ts: " << img->getTimestamp().time_since_epoch().count() 
                                   << " Size: " << img->getData().size() << " bytes\n";
                    } else {
                         std::cout << "  " << name << " MISSING!\n";
                    }
                }
                received = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if(!received) {
            std::cout << "FAILURE: Quad Sync (MJPEG) timeout\n";
            return false;
        }
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        return false;
    }
    return true;
}

int main() {
    std::cout << "MiceCam OAK Hardware Diagnostic Tool\n";
    std::cout << "====================================\n";
    
    std::vector<std::pair<dai::CameraBoardSocket, std::string>> tests = {
        {dai::CameraBoardSocket::CAM_A, "CAM_A"},
        {dai::CameraBoardSocket::CAM_B, "CAM_B"},
        {dai::CameraBoardSocket::CAM_C, "CAM_C"},
        {dai::CameraBoardSocket::CAM_D, "CAM_D"}
    };
    
    int passes = 0;
    for(const auto& t : tests) {
        if(test_socket(t.first, t.second)) {
            passes++;
        }
    }
    
    std::cout << "\n====================================\n";
    std::cout << "SUMMARY: " << passes << "/4 Cameras Functional\n";
    
    if(passes == 4) {
        if(test_sync_pair(dai::CameraBoardSocket::CAM_A, dai::CameraBoardSocket::CAM_B)) {
            test_quad_sync();
        }
    }
    
    return (passes == 4) ? 0 : 1;
}

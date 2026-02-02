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
    
    return (passes == 4) ? 0 : 1;
}

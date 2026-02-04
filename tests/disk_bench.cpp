#include "micecam/pipeline/disk_writer.h"
#include "micecam/core/ring_buffer.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <filesystem>

using namespace micecam;

int main() {
    std::filesystem::create_directories("test_output");
    
    SessionConfig config;
    config.output_dir = "test_output";
    config.session_name = "disk_bench";
    config.enable_checksums = false;
    
    DiskWriter writer(config);
    RingBuffer buffer(600);
    
    const size_t frame_size = 1920 * 1080 * 3 / 10; // ~0.6MB simulated MJPEG
    const int total_frames = 10000;
    
    std::cout << "Starting DiskWriter benchmark..." << std::endl;
    if (!writer.start()) return 1;
    writer.consume_from(buffer);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < total_frames; ++i) {
        auto data = std::make_unique<std::vector<uint8_t>>(frame_size, 0xA5);
        Frame frame(i, std::move(data));
        
        while (!buffer.try_push(std::move(frame))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        if (i % 100 == 0) std::cout << "Pushed " << i << " frames..." << std::endl;
    }
    
    writer.stop();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double speed = (total_frames * frame_size) / (1024.0 * 1024.0) / (duration / 1000.0);
    
    std::cout << "Done." << std::endl;
    std::cout << "Avg Write Speed: " << speed << " MB/s" << std::endl;
    
    return 0;
}

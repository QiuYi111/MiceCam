#include "micecam/core/ring_buffer.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>

using namespace micecam;

int main() {
    const size_t buffer_size = 600;
    const size_t frame_size = 1920 * 1080 * 3; // ~6MB (BGR 1080p)
    const int total_frames = 3000; // ~50 seconds at 60fps
    
    RingBuffer buffer(buffer_size);
    std::atomic<int> processed{0};
    
    std::cout << "Starting RingBuffer benchmark (No Disk I/O)..." << std::endl;
    std::cout << "Frame size: " << frame_size / (1024.0 * 1024.0) << " MB" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Consumer thread (just pops and counts)
    std::thread consumer([&]() {
        while (processed < total_frames) {
            auto frame = buffer.try_pop();
            if (frame) {
                processed++;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    });
    
    // Producer (pushes as fast as possible or simulated 60fps)
    for (int i = 0; i < total_frames; ++i) {
        auto data = std::make_unique<std::vector<uint8_t>>(frame_size, 0);
        Frame frame(i, std::move(data));
        
        while (!buffer.try_push(std::move(frame))) {
            // Buffer full, wait a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    consumer.join();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double throughput = (total_frames * frame_size) / (1024.0 * 1024.0 * 1024.0) / (duration / 1000.0);
    
    std::cout << "Done." << std::endl;
    std::cout << "Duration: " << duration << " ms" << std::endl;
    std::cout << "Throughput: " << throughput << " GB/s" << std::endl;
    
    return 0;
}

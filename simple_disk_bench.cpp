#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

int main() {
    const size_t chunk_size = 8 * 1024 * 1024; // 8MB chunks
    const size_t total_size = 1024 * 1024 * 1024; // 1GB total
    std::vector<char> buffer(chunk_size, 0);
    
    std::ofstream out("test_io_big.bin", std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "Failed to open file" << std::endl;
        return 1;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < total_size / chunk_size; ++i) {
        out.write(buffer.data(), buffer.size());
    }
    out.flush();
    auto end = std::chrono::high_resolution_clock::now();
    
    out.close();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double speed = (total_size / (1024.0 * 1024.0)) / (duration / 1000.0);
    
    std::cout << "Write Speed (8MB blocks): " << speed << " MB/s" << std::endl;
    
    return 0;
}

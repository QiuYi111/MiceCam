#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace micecam::infrastructure {

class SharedMemoryBackend {
public:
    virtual ~SharedMemoryBackend() = default;
    virtual int open(const std::string& name, std::size_t size) = 0;
    virtual void* map(int fd, std::size_t size) = 0;
    virtual void unmap(void* ptr, std::size_t size) = 0;
    virtual void unlink(const std::string& name) = 0;
    virtual void close(int fd) = 0;
};

#ifdef _WIN32
inline constexpr const char* kShmTransportType = "win32_mapping";
#else
inline constexpr const char* kShmTransportType = "posix_shm";
#endif

std::unique_ptr<SharedMemoryBackend> create_shared_memory_backend();

} // namespace micecam::infrastructure

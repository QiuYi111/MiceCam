#pragma once

#ifdef _WIN32

#include "infrastructure/SharedMemoryBackend.h"

namespace micecam::infrastructure {

class Win32SharedMemory : public SharedMemoryBackend {
public:
    int open(const std::string& name, std::size_t size) override;
    void* map(int fd, std::size_t size) override;
    void unmap(void* ptr, std::size_t size) override;
    void unlink(const std::string& name) override;
    void close(int fd) override;
};

} // namespace micecam::infrastructure

#endif // _WIN32

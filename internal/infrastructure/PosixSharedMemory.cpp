#ifndef _WIN32

#include "infrastructure/PosixSharedMemory.h"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace micecam::infrastructure {

int PosixSharedMemory::open(const std::string& name, std::size_t size) {
    int fd;
    if (size > 0) {
        fd = shm_open(name.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600);
        if (fd < 0) {
            fd = shm_open(name.c_str(), O_RDWR, 0600);
            if (fd < 0) {
                spdlog::error("PosixSharedMemory: shm_open failed for {}: {}", name, strerror(errno));
                return -1;
            }
        }
        if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
            spdlog::error("PosixSharedMemory: ftruncate failed for {}: {}", name, strerror(errno));
            ::close(fd);
            shm_unlink(name.c_str());
            return -1;
        }
    } else {
        fd = shm_open(name.c_str(), O_RDWR, 0);
        if (fd < 0) {
            spdlog::error("PosixSharedMemory: shm_open failed for {}: {}", name, strerror(errno));
            return -1;
        }
    }
    return fd;
}

void* PosixSharedMemory::map(int fd, std::size_t size) {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        spdlog::error("PosixSharedMemory: mmap failed: {}", strerror(errno));
        return nullptr;
    }
    return ptr;
}

void PosixSharedMemory::unmap(void* ptr, std::size_t size) {
    munmap(ptr, size);
}

void PosixSharedMemory::unlink(const std::string& name) {
    shm_unlink(name.c_str());
}

void PosixSharedMemory::close(int fd) {
    ::close(fd);
}

} // namespace micecam::infrastructure

#endif // !_WIN32

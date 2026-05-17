#include "infrastructure/SharedMemoryBackend.h"

#ifdef _WIN32
#include "infrastructure/Win32SharedMemory.h"
#else
#include "infrastructure/PosixSharedMemory.h"
#endif

namespace micecam::infrastructure {

std::unique_ptr<SharedMemoryBackend> create_shared_memory_backend() {
#ifdef _WIN32
    return std::make_unique<Win32SharedMemory>();
#else
    return std::make_unique<PosixSharedMemory>();
#endif
}

} // namespace micecam::infrastructure

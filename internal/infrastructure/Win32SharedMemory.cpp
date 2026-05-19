#ifdef _WIN32

#include "infrastructure/Win32SharedMemory.h"

#include <spdlog/spdlog.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace micecam::infrastructure {

static std::string make_win32_name(const std::string& name) {
    std::string clean = name;
    if (!clean.empty() && clean[0] == '/') {
        clean = clean.substr(1);
    }
    return "Global\\MiceCam_" + clean;
}

int Win32SharedMemory::open(const std::string& name, std::size_t size) {
    std::string win_name = make_win32_name(name);
    HANDLE hMapping;
    if (size > 0) {
        hMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                      0, static_cast<DWORD>(size), win_name.c_str());
        if (!hMapping) {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS) {
                spdlog::error("Win32SharedMemory: CreateFileMapping failed for {}: error {}",
                              win_name, err);
                return -1;
            }
            hMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, win_name.c_str());
            if (!hMapping) {
                spdlog::error("Win32SharedMemory: OpenFileMapping failed for {}: error {}",
                              win_name, GetLastError());
                return -1;
            }
        }
    } else {
        hMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, win_name.c_str());
        if (!hMapping) {
            spdlog::error("Win32SharedMemory: OpenFileMapping failed for {}: error {}",
                          win_name, GetLastError());
            return -1;
        }
    }
    return static_cast<int>(reinterpret_cast<intptr_t>(hMapping));
}

void* Win32SharedMemory::map(int fd, std::size_t size) {
    HANDLE hMapping = reinterpret_cast<HANDLE>(static_cast<intptr_t>(fd));
    void* ptr = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!ptr) {
        spdlog::error("Win32SharedMemory: MapViewOfFile failed: error {}", GetLastError());
        return nullptr;
    }
    return ptr;
}

void Win32SharedMemory::unmap(void* ptr, std::size_t /*size*/) {
    UnmapViewOfFile(ptr);
}

void Win32SharedMemory::unlink(const std::string& /*name*/) {
}

void Win32SharedMemory::close(int fd) {
    HANDLE hMapping = reinterpret_cast<HANDLE>(static_cast<intptr_t>(fd));
    CloseHandle(hMapping);
}

} // namespace micecam::infrastructure

#endif // _WIN32

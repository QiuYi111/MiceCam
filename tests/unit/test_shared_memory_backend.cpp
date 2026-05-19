#ifndef _WIN32

#include <gtest/gtest.h>

#include <cstring>

#include "infrastructure/PosixSharedMemory.h"

namespace {

class PosixSharedMemoryTest : public ::testing::Test {
protected:
    void TearDown() override {
        for (auto& name : created_names_) {
            shm_.unlink(name);
        }
    }

    std::string unique_name(const char* suffix) {
        std::string name = std::string("/micecam_test_") + suffix + "_" +
                           std::to_string(::getpid()) + "_" +
                           std::to_string(counter_++);
        created_names_.push_back(name);
        return name;
    }

    micecam::infrastructure::PosixSharedMemory shm_;
    std::vector<std::string> created_names_;
    static inline int counter_ = 0;
};

TEST_F(PosixSharedMemoryTest, OpenMapWriteReadUnmapUnlinkCycle) {
    std::string name = unique_name("cycle");
    constexpr std::size_t kSize = 4096;

    int fd = shm_.open(name, kSize);
    ASSERT_GE(fd, 0);

    void* ptr = shm_.map(fd, kSize);
    ASSERT_NE(ptr, nullptr);

    std::memset(ptr, 0xAB, kSize);
    auto* data = static_cast<unsigned char*>(ptr);
    EXPECT_EQ(data[0], 0xAB);
    EXPECT_EQ(data[kSize - 1], 0xAB);

    shm_.unmap(ptr, kSize);
    shm_.close(fd);
}

TEST_F(PosixSharedMemoryTest, ReOpenExistingNameSucceeds) {
    std::string name = unique_name("collide");
    constexpr std::size_t kSize = 1024;

    int fd1 = shm_.open(name, kSize);
    ASSERT_GE(fd1, 0);

    void* ptr1 = shm_.map(fd1, kSize);
    ASSERT_NE(ptr1, nullptr);
    std::memset(ptr1, 0x42, kSize);
    shm_.unmap(ptr1, kSize);
    shm_.close(fd1);

    int fd2 = shm_.open(name, 0);
    ASSERT_GE(fd2, 0);

    void* ptr2 = shm_.map(fd2, kSize);
    ASSERT_NE(ptr2, nullptr);
    auto* data = static_cast<unsigned char*>(ptr2);
    EXPECT_EQ(data[0], 0x42);

    shm_.unmap(ptr2, kSize);
    shm_.close(fd2);
}

TEST_F(PosixSharedMemoryTest, UnlinkNonExistentNoCrash) {
    shm_.unlink("/micecam_test_nonexistent_99999");
    SUCCEED();
}

TEST_F(PosixSharedMemoryTest, MultipleRegionsCoexist) {
    std::string name_a = unique_name("multi_a");
    std::string name_b = unique_name("multi_b");
    constexpr std::size_t kSize = 512;

    int fd_a = shm_.open(name_a, kSize);
    ASSERT_GE(fd_a, 0);
    int fd_b = shm_.open(name_b, kSize);
    ASSERT_GE(fd_b, 0);

    void* ptr_a = shm_.map(fd_a, kSize);
    void* ptr_b = shm_.map(fd_b, kSize);
    ASSERT_NE(ptr_a, nullptr);
    ASSERT_NE(ptr_b, nullptr);

    std::memset(ptr_a, 0x11, kSize);
    std::memset(ptr_b, 0x22, kSize);

    EXPECT_EQ(static_cast<unsigned char*>(ptr_a)[0], 0x11);
    EXPECT_EQ(static_cast<unsigned char*>(ptr_b)[0], 0x22);

    shm_.unmap(ptr_a, kSize);
    shm_.unmap(ptr_b, kSize);
    shm_.close(fd_a);
    shm_.close(fd_b);
}

} // namespace

#endif // !_WIN32

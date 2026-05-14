#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "infrastructure/StreamWriter.h"

using namespace micecam::infrastructure;

namespace {

const char* TEST_OUTPUT = "test_output";

bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

} // namespace

TEST(StreamWriter, OpenCreatesFile) {
    std::string path = std::string(TEST_OUTPUT) + "/sw_test_open.mp4";
    StreamWriter writer;
    ASSERT_TRUE(writer.open(path, 320, 240, 30));
    EXPECT_TRUE(file_exists(path));
    writer.close();
}

TEST(StreamWriter, WritePacketWorks) {
    std::string path = std::string(TEST_OUTPUT) + "/sw_test_write.mp4";
    StreamWriter writer;
    ASSERT_TRUE(writer.open(path, 320, 240, 30));

    std::vector<uint8_t> dummy_packet(256, 0x00);
    EXPECT_TRUE(writer.write_packet(dummy_packet.data(), dummy_packet.size(), 0, 0, true));
    writer.close();
}

TEST(StreamWriter, CloseProducesNonEmptyFile) {
    std::string path = std::string(TEST_OUTPUT) + "/sw_test_close.mp4";
    StreamWriter writer;
    ASSERT_TRUE(writer.open(path, 320, 240, 30));
    writer.close();

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(f.good());
    auto size = f.tellg();
    EXPECT_GT(size, 0);
}

TEST(StreamWriter, WriteMultiplePacketsThenClose) {
    std::string path = std::string(TEST_OUTPUT) + "/sw_test_multi.mp4";
    StreamWriter writer;
    ASSERT_TRUE(writer.open(path, 320, 240, 30));

    std::vector<uint8_t> dummy_packet(512, 0x00);
    for (int i = 0; i < 10; i++) {
        writer.write_packet(dummy_packet.data(), dummy_packet.size(), i, i, (i % 5 == 0));
    }
    writer.close();

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(f.good());
    EXPECT_GT(f.tellg(), 0);
}

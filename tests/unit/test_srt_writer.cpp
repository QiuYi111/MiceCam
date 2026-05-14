#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "domain/FrameTimestamp.h"
#include "infrastructure/SRTWriter.h"

using namespace micecam;

namespace {

const char* TEST_OUTPUT = "test_output";

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

} // namespace

TEST(SRTWriter, OpenCreatesFile) {
    std::string path = std::string(TEST_OUTPUT) + "/test_open.srt";
    infrastructure::SRTWriter writer;
    EXPECT_TRUE(writer.open(path));
    writer.close();

    std::ifstream f(path);
    EXPECT_TRUE(f.good());
}

TEST(SRTWriter, WriteEntryProducesCorrectFormat) {
    std::string path = std::string(TEST_OUTPUT) + "/test_entry.srt";
    infrastructure::SRTWriter writer;
    ASSERT_TRUE(writer.open(path));

    domain::FrameTimestamp ts;
    ts.session_offset_us = 0;
    ts.hardware_pts = 0;
    ts.has_hardware_pts = false;

    writer.write_entry(1, ts, false);
    writer.close();

    auto content = read_file(path);
    EXPECT_TRUE(content.find("1") != std::string::npos);
    EXPECT_TRUE(content.find("00:00:00,000") != std::string::npos);
    EXPECT_TRUE(content.find("seq=1") != std::string::npos);
    EXPECT_TRUE(content.find("skipped=false") != std::string::npos);
}

TEST(SRTWriter, WriteMultipleEntries) {
    std::string path = std::string(TEST_OUTPUT) + "/test_multi.srt";
    infrastructure::SRTWriter writer;
    ASSERT_TRUE(writer.open(path));

    for (uint64_t i = 0; i < 5; i++) {
        domain::FrameTimestamp ts;
        ts.session_offset_us = i * 33333;
        ts.has_hardware_pts = false;

        writer.write_entry(i, ts, (i == 2));
    }
    writer.close();

    auto content = read_file(path);
    EXPECT_GT(content.size(), 0);
    EXPECT_TRUE(content.find("seq=0") != std::string::npos);
    EXPECT_TRUE(content.find("seq=4") != std::string::npos);
    EXPECT_TRUE(content.find("skipped=true") != std::string::npos);
}

#include <gtest/gtest.h>

#include <fstream>
#include <regex>
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

TEST(SRTWriter, WallTimeInISO8601Format) {
    std::string path = std::string(TEST_OUTPUT) + "/test_walltime.srt";
    infrastructure::SRTWriter writer;
    ASSERT_TRUE(writer.open(path));

    domain::FrameTimestamp ts;
    ts.session_offset_us = 1000000;
    ts.wall_time_ns = 1747492200000000000ULL; // a known timestamp

    writer.write_entry(1, ts, false);
    writer.close();

    auto content = read_file(path);
    EXPECT_TRUE(content.find("wall_time=") != std::string::npos);

    auto pos = content.find("wall_time=");
    ASSERT_NE(pos, std::string::npos);
    std::string wt_value = content.substr(pos + 10, 26);
    EXPECT_EQ(wt_value.size(), 26);

    EXPECT_TRUE(wt_value[4] == '-');
    EXPECT_TRUE(wt_value[7] == '-');
    EXPECT_TRUE(wt_value[10] == 'T');
    EXPECT_TRUE(wt_value[13] == ':');
    EXPECT_TRUE(wt_value[16] == ':');
    EXPECT_TRUE(wt_value[19] == '.');
}

TEST(SRTWriter, NoWallTimeWhenZero) {
    std::string path = std::string(TEST_OUTPUT) + "/test_no_walltime.srt";
    infrastructure::SRTWriter writer;
    ASSERT_TRUE(writer.open(path));

    domain::FrameTimestamp ts;
    ts.session_offset_us = 0;
    ts.wall_time_ns = 0;

    writer.write_entry(1, ts, false);
    writer.close();

    auto content = read_file(path);
    EXPECT_TRUE(content.find("wall_time=") == std::string::npos);
}

TEST(SRTWriter, FpsParameterAffectsDuration) {
    std::string path60 = std::string(TEST_OUTPUT) + "/test_fps60.srt";
    std::string path30 = std::string(TEST_OUTPUT) + "/test_fps30.srt";

    {
        infrastructure::SRTWriter writer;
        ASSERT_TRUE(writer.open(path60, 60.0));
        domain::FrameTimestamp ts;
        ts.session_offset_us = 0;
        writer.write_entry(1, ts, false);
        writer.close();
    }
    {
        infrastructure::SRTWriter writer;
        ASSERT_TRUE(writer.open(path30, 30.0));
        domain::FrameTimestamp ts;
        ts.session_offset_us = 0;
        writer.write_entry(1, ts, false);
        writer.close();
    }

    auto c60 = read_file(path60);
    auto c30 = read_file(path30);

    // At 60fps: duration = 16667us → end_time "00:00:00,016"
    // At 30fps: duration = 33333us → end_time "00:00:00,033"
    EXPECT_TRUE(c60.find("00:00:00,016") != std::string::npos);
    EXPECT_TRUE(c30.find("00:00:00,033") != std::string::npos);
}

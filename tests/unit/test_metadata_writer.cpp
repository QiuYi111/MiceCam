#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "domain/SessionMetadata.h"
#include "domain/StreamStats.h"
#include "infrastructure/MetadataWriter.h"

using namespace micecam;

namespace {

const char* TEST_OUTPUT = "test_output";

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

} // namespace

TEST(MetadataWriter, WriteSessionHeaderCreatesValidJson) {
    std::string path = std::string(TEST_OUTPUT) + "/test_meta.json";

    domain::SessionMetadata meta;
    meta.session_id = "test-session-001";
    meta.wall_clock_anchor_ns = 1234567890;
    meta.encoder_name = "libx264";
    meta.bitrate_kbps = 5000;
    meta.keyframe_interval = 60;
    meta.output_dir = "/tmp/test";
    meta.start_time_ns = 1000000;
    meta.end_time_ns = 0;

    EXPECT_TRUE(infrastructure::MetadataWriter::write_session_header(meta, path));

    auto content = read_file(path);
    auto j = nlohmann::json::parse(content);
    EXPECT_EQ(j["session_id"], "test-session-001");
    EXPECT_EQ(j["encoder_name"], "libx264");
    EXPECT_EQ(j["bitrate_kbps"], 5000);
}

TEST(MetadataWriter, WriteSessionFooterUpdatesFile) {
    std::string path = std::string(TEST_OUTPUT) + "/test_meta_footer.json";

    domain::SessionMetadata meta;
    meta.session_id = "test-session-002";
    meta.output_dir = "/tmp/test";
    meta.encoder_name = "libx264";
    infrastructure::MetadataWriter::write_session_header(meta, path);

    EXPECT_TRUE(infrastructure::MetadataWriter::write_session_footer(
        path, 300, 5000000, "abc123def"));

    auto content = read_file(path);
    auto j = nlohmann::json::parse(content);
    EXPECT_EQ(j["total_frames"], 300);
    EXPECT_EQ(j["total_bytes"], 5000000);
    EXPECT_EQ(j["session_checksum"], "abc123def");
}

TEST(MetadataWriter, WriteStatsCreatesValidJson) {
    std::string path = std::string(TEST_OUTPUT) + "/test_stats.json";

    std::vector<domain::StreamStats> stats;
    domain::StreamStats s;
    s.stream_id = "cam0";
    s.frames_expected = 100;
    s.frames_actual = 99;
    s.drop_rate = 0.01;
    s.bytes_written = 500000;
    s.encoder_used = "h264_videotoolbox";
    s.encoder_fallback = false;
    stats.push_back(s);

    domain::StreamStats s2;
    s2.stream_id = "cam1";
    s2.frames_expected = 100;
    s2.frames_actual = 100;
    s2.encoder_used = "libx264";
    s2.encoder_fallback = true;
    stats.push_back(s2);

    EXPECT_TRUE(infrastructure::MetadataWriter::write_stats(path, stats));

    auto content = read_file(path);
    auto j = nlohmann::json::parse(content);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2);
    EXPECT_EQ(j[0]["stream_id"], "cam0");
    EXPECT_EQ(j[0]["encoder_used"], "h264_videotoolbox");
    EXPECT_EQ(j[1]["encoder_fallback"], true);
}

TEST(MetadataWriter, SessionHeaderJsonIsPrettyPrinted) {
    std::string path = std::string(TEST_OUTPUT) + "/test_pretty.json";

    domain::SessionMetadata meta;
    meta.session_id = "test-pretty";
    meta.encoder_name = "libx264";

    EXPECT_TRUE(infrastructure::MetadataWriter::write_session_header(meta, path));

    auto content = read_file(path);
    EXPECT_TRUE(content.find('\n') != std::string::npos);
    auto j = nlohmann::json::parse(content);
    EXPECT_EQ(j["session_id"], "test-pretty");
}

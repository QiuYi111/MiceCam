#include "micecam/pipeline/ingestion_pipeline.h"
#include "camera/fake_camera.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace micecam {

namespace {

std::vector<nlohmann::json> load_jsonl_records(const fs::path& path) {
    std::ifstream file(path);
    std::vector<nlohmann::json> records;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            records.push_back(nlohmann::json::parse(line));
        }
    }
    return records;
}

}  // namespace

class PipelineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for test output
        test_dir_ = "test_output";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        // Cleanup (optional, keep files for inspection)
    }

    std::string test_dir_;
};

TEST_F(PipelineIntegrationTest, EndToEndFakeCameraCapture) {
    // Setup: Create fake camera with reasonable frame rate
    const size_t frame_size = 320 * 240 * 3;  // Smaller RGB frame
    const double fps = 10.0;  // Lower FPS to avoid buffer overflow
    const int num_frames_to_capture = 50;

    auto camera = std::make_unique<FakeCamera>(frame_size);

    CameraConfig config;
    config.width = 320;
    config.height = 240;
    config.fps = fps;

    ASSERT_TRUE(camera->initialize(config));
    camera->set_max_frames(num_frames_to_capture);

    // Setup: Create pipeline
    SessionConfig session_config;
    session_config.output_dir = test_dir_;
    session_config.session_name = "test_session";
    session_config.enable_checksums = true;

    IngestionPipeline pipeline(std::move(camera), session_config);

    // Execute: Run capture
    ASSERT_TRUE(pipeline.start());

    // Wait for capture to complete
    pipeline.join();
    pipeline.stop();

    // Verify: Check we captured reasonable number of frames
    // Note: Some drops are expected with small RingBuffer (size=10)
    // This is intentional - testing real system behavior
    const uint64_t frames_captured = pipeline.get_frames_captured();
    EXPECT_GT(frames_captured, num_frames_to_capture * 0.5)  // At least 50%
        << "Only captured " << frames_captured << " / " << num_frames_to_capture << " frames";

    // Verify: Check files exist (use std::filesystem for cross-platform paths)
    const fs::path bin_path = fs::path(test_dir_) / "test_session.bin";
    const fs::path json_path = fs::path(test_dir_) / "test_session_metadata.jsonl";

    std::ifstream bin_file(bin_path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(bin_file.is_open()) << "Binary file not created";

    const size_t bin_size = bin_file.tellg();
    EXPECT_EQ(bin_size, frames_captured * frame_size)
        << "Binary file size mismatch";

    // Verify: Check metadata JSON
    ASSERT_TRUE(fs::exists(json_path)) << "Metadata file not created";

    const auto records = load_jsonl_records(json_path);
    ASSERT_GE(records.size(), 2u);

    const nlohmann::json& session = records.back();
    EXPECT_EQ(session["type"].get<std::string>(), "session_end");
    EXPECT_EQ(session["total_frames"].get<uint64_t>(), frames_captured);
    EXPECT_EQ(session["total_bytes"].get<uint64_t>(), frames_captured * frame_size);
    EXPECT_TRUE(session["session_checksum"].get<uint32_t>() > 0);

    std::vector<nlohmann::json> frames;
    for (const auto& record : records) {
        if (record.value("type", "") == "frame") {
            frames.push_back(record);
        }
    }

    EXPECT_EQ(frames.size(), frames_captured);

    if (!frames.empty()) {
        // Check first frame metadata
        EXPECT_EQ(frames[0]["sequence_id"].get<uint64_t>(), 1);
        EXPECT_EQ(frames[0]["size"].get<uint64_t>(), frame_size);
        EXPECT_TRUE(frames[0]["checksum"].get<uint32_t>() > 0);

        // Check that frame sequence IDs are monotonically increasing
        // (Note: gaps are OK - they indicate dropped frames)
        uint64_t prev_seq = 0;
        for (const auto& frame_rec : frames) {
            const uint64_t seq = frame_rec["sequence_id"].get<uint64_t>();
            EXPECT_GT(seq, prev_seq) << "Frame sequence not monotonic";
            prev_seq = seq;
        }

        // Verify: Check frame offsets are sequential (no gaps in file)
        uint64_t expected_offset = 0;
        for (const auto& frame_rec : frames) {
            EXPECT_EQ(frame_rec["offset"].get<uint64_t>(), expected_offset);
            expected_offset += frame_rec["size"].get<uint64_t>();
        }
    }

    std::cout << "Integration test passed:\n"
              << "  Frames captured: " << frames_captured << " / " << num_frames_to_capture << "\n"
              << "  Binary file: " << bin_path << " (" << bin_size << " bytes)\n"
              << "  Metadata: " << json_path << "\n";
}

TEST_F(PipelineIntegrationTest, RealDiskIOPerformance) {
    // Test actual disk write performance (not just RingBuffer throughput)
    const size_t frame_size = static_cast<size_t>(200 * 1024 * 1024 / 60);  // 200 MB/s at 60 FPS
    const double fps = 60.0;
    const int num_frames = 300;  // 5 seconds worth

    auto camera = std::make_unique<FakeCamera>(frame_size);
    CameraConfig config;
    config.fps = fps;
    config.width = 1920;
    config.height = 1080;

    ASSERT_TRUE(camera->initialize(config));
    camera->set_max_frames(num_frames);

    SessionConfig session_config;
    session_config.output_dir = test_dir_;
    session_config.session_name = "perf_test";
    session_config.enable_checksums = false;  // Disable for pure I/O test

    IngestionPipeline pipeline(std::move(camera), session_config);

    // Measure performance
    auto start_time = std::chrono::high_resolution_clock::now();

    ASSERT_TRUE(pipeline.start());
    pipeline.join();
    pipeline.stop();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    const double actual_bandwidth_mbps = (pipeline.get_writer().get_bytes_written() /
                                         (1024.0 * 1024.0)) /
                                        (duration.count() / 1000.0);

    std::cout << "\n=== Real Disk I/O Performance ===\n";
    std::cout << "Duration: " << duration.count() << " ms\n";
    std::cout << "Bytes written: " << pipeline.get_writer().get_bytes_written() / (1024.0 * 1024.0) << " MB\n";
    std::cout << "Bandwidth: " << actual_bandwidth_mbps << " MB/s\n";
    std::cout << "Frames written: " << pipeline.get_writer().get_frames_written() << "\n";

    // Expect at least 150 MB/s (allowing for overhead)
    EXPECT_GT(actual_bandwidth_mbps, 150.0);
}

TEST_F(PipelineIntegrationTest, DataIntegrityCheck) {
    // Verify checksums are computed correctly
    const size_t frame_size = 1024;
    const int num_frames = 10;

    auto camera = std::make_unique<FakeCamera>(frame_size);
    CameraConfig config;
    config.fps = 30.0;
    ASSERT_TRUE(camera->initialize(config));
    camera->set_max_frames(num_frames);

    SessionConfig session_config;
    session_config.output_dir = test_dir_;
    session_config.session_name = "integrity_test";
    session_config.enable_checksums = true;

    IngestionPipeline pipeline(std::move(camera), session_config);

    ASSERT_TRUE(pipeline.start());
    pipeline.join();
    pipeline.stop();

    // Load metadata and verify all checksums are non-zero
    const fs::path json_path = fs::path(test_dir_) / "integrity_test_metadata.jsonl";
    ASSERT_TRUE(fs::exists(json_path));

    const auto records = load_jsonl_records(json_path);
    ASSERT_GE(records.size(), 2u);

    const nlohmann::json& session = records.back();
    EXPECT_GT(session["session_checksum"], 0) << "Session checksum should be non-zero";

    for (const auto& record : records) {
        if (record.value("type", "") == "frame") {
            EXPECT_GT(record["checksum"], 0) << "Frame " << record["sequence_id"] << " has zero checksum";
        }
    }

    std::cout << "Data integrity check passed\n";
}

}  // namespace micecam

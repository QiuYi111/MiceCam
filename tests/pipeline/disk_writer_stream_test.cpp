#include "micecam/pipeline/disk_writer.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>

namespace fs = std::filesystem;

namespace micecam {

class DiskWriterStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "test_stream_output";
        std::filesystem::create_directories(test_dir_);

        // Clean previous
        fs::remove_all(test_dir_);
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        // fs::remove_all(test_dir_);
    }

    std::string test_dir_;
};

TEST_F(DiskWriterStreamTest, StreamsMetadataToJsonl) {
    SessionConfig config;
    config.output_dir = test_dir_;
    config.session_name = "stream_test";
    config.camera_backend_name = "MockBackend";
    config.width = 640;
    config.height = 480;
    config.fps = 30.0;
    config.ring_buffer_size = 10;
    config.enable_checksums = false; // simplify

    DiskWriter writer(config);
    RingBuffer buffer(10);

    // 1. Initial Start -> Should create file and write header
    ASSERT_TRUE(writer.start());

    fs::path jsonl_path = fs::path(test_dir_) / "stream_test_metadata.jsonl";
    ASSERT_TRUE(fs::exists(jsonl_path));

    // Verify Header
    {
        std::ifstream f(jsonl_path);
        std::string line;
        ASSERT_TRUE(std::getline(f, line));
        auto j = nlohmann::json::parse(line);
        EXPECT_EQ(j["type"], "session_start");
        EXPECT_EQ(j["camera_backend"], "MockBackend");
    }

    // 2. Consume Frames
    writer.consume_from(buffer);

    // Push 3 frames
    std::vector<uint8_t> dummy_data(100, 0xAB);
    for(int i=1; i<=3; ++i) {
        auto data_ptr = std::make_unique<std::vector<uint8_t>>(dummy_data);
        Frame frame;
        frame.data = std::move(data_ptr);
        frame.sequence_id = i;
        frame.timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now()
        );
        buffer.push(std::move(frame));
    }

    // Wait for writer to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify valid streaming content while running
    {
        std::ifstream f(jsonl_path);
        std::string line;

        // Header
        std::getline(f, line);

        // Frames
        int count = 0;
        while(std::getline(f, line)) {
            if (line.empty()) continue;
            auto j = nlohmann::json::parse(line);
            if (j["type"] == "frame") {
                count++;
                EXPECT_EQ(j["sequence_id"], count);
            }
        }
        EXPECT_EQ(count, 3) << "Should have streamed 3 frames to disk";
    }

    // 3. Stop
    writer.stop();

    // Verify Footer
    {
        std::ifstream f(jsonl_path);
        std::string line;
        std::string last_line;
        while(std::getline(f, line)) {
            if(!line.empty()) last_line = line;
        }
        auto j = nlohmann::json::parse(last_line);
        EXPECT_EQ(j["type"], "session_end");
        EXPECT_EQ(j["total_frames"], 3);
    }
}

} // namespace micecam

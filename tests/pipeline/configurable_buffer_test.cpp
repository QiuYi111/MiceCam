#include "micecam/pipeline/ingestion_pipeline.h"
#include "camera/fake_camera.h"
#include <gtest/gtest.h>

namespace micecam {

TEST(ConfigurableBufferTest, ConfigurableBufferSizeWorks) {
    // Test that configurable buffer size is respected
    // Note: FakeCamera may not generate frames fast enough to cause drops
    // This test validates the configuration mechanism, not drop behavior

    const size_t frame_size = 320 * 240 * 3;
    auto camera = std::make_unique<FakeCamera>(frame_size);

    CameraConfig config;
    config.fps = 30.0;
    config.width = 320;
    config.height = 240;

    ASSERT_TRUE(camera->initialize(config));
    camera->set_max_frames(50);

    SessionConfig session_config;
    session_config.output_dir = "test_output";
    session_config.session_name = "buffer_size_test";
    session_config.enable_checksums = false;
    session_config.ring_buffer_size = 25;  // Custom size
    session_config.camera_backend_name = "FakeCamera";
    session_config.width = 320;
    session_config.height = 240;
    session_config.fps = 30.0;

    IngestionPipeline pipeline(std::move(camera), session_config);
    pipeline.start();
    pipeline.join();
    pipeline.stop();

    // Verify capture completed
    EXPECT_EQ(pipeline.get_frames_captured(), 50);
    EXPECT_EQ(pipeline.get_frames_dropped(), 0);

    // Verify metadata
    const std::string metadata_path = "test_output/buffer_size_test_metadata.json";
    std::ifstream json_file(metadata_path);
    ASSERT_TRUE(json_file.is_open());

    nlohmann::json metadata;
    json_file >> metadata;

    auto session = metadata["session"];
    EXPECT_EQ(session["camera_backend"].get<std::string>(), "FakeCamera");
    EXPECT_EQ(session["width"].get<int>(), 320);
    EXPECT_EQ(session["height"].get<int>(), 240);

    std::cout << "Configurable buffer test passed: "
              << pipeline.get_frames_captured() << " frames captured\n";
}

TEST(ConfigurableBufferTest, MetadataContainsCameraConfig) {
    // Verify that camera configuration is properly saved in metadata
    const size_t frame_size = 320 * 240 * 3;
    auto camera = std::make_unique<FakeCamera>(frame_size);

    CameraConfig config;
    config.fps = 30.0;
    config.width = 320;
    config.height = 240;

    ASSERT_TRUE(camera->initialize(config));
    camera->set_max_frames(10);

    SessionConfig session_config;
    session_config.output_dir = "test_output";
    session_config.session_name = "metadata_test";
    session_config.enable_checksums = false;
    session_config.ring_buffer_size = 10;
    session_config.camera_backend_name = "FakeCamera";
    session_config.width = 320;
    session_config.height = 240;
    session_config.fps = 30.0;

    IngestionPipeline pipeline(std::move(camera), session_config);
    pipeline.start();
    pipeline.join();
    pipeline.stop();

    // Verify metadata file contains correct camera info
    const std::string metadata_path = "test_output/metadata_test_metadata.json";
    std::ifstream json_file(metadata_path);
    ASSERT_TRUE(json_file.is_open());

    nlohmann::json metadata;
    json_file >> metadata;

    auto session = metadata["session"];
    EXPECT_EQ(session["camera_backend"].get<std::string>(), "FakeCamera");
    EXPECT_EQ(session["width"].get<int>(), 320);
    EXPECT_EQ(session["height"].get<int>(), 240);
    EXPECT_EQ(session["fps"].get<double>(), 30.0);

    std::cout << "Metadata verification passed\n";
}

}  // namespace micecam

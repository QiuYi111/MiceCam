#include <gtest/gtest.h>

#include <thread>

#include "domain/CameraStream.h"
#include "domain/DeviceInfo.h"
#include "domain/StreamConfig.h"
#include "infrastructure/MockCameraBackend.h"

using namespace micecam;

TEST(MockCameraBackend, BackendNameIsMock) {
    infrastructure::MockCameraBackend backend;
    EXPECT_EQ(backend.backend_name(), "Mock");
}

TEST(MockCameraBackend, EnumerateReturnsDevices) {
    infrastructure::MockCameraBackend backend;
    auto devices = backend.enumerate_devices();
    EXPECT_GE(devices.size(), 1u);
    EXPECT_FALSE(devices[0].id.empty());
    EXPECT_FALSE(devices[0].name.empty());
    EXPECT_EQ(devices[0].type, "mock");
}

TEST(MockCameraBackend, OpenStreamSucceeds) {
    infrastructure::MockCameraBackend backend;
    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 640;
    config.height = 480;
    config.framerate = 30;

    auto stream = backend.open_stream(config);
    ASSERT_NE(stream, nullptr);
    EXPECT_TRUE(stream->is_open());
    EXPECT_EQ(stream->width(), 640);
    EXPECT_EQ(stream->height(), 480);
    EXPECT_EQ(stream->fps(), 30);
}

TEST(MockCameraBackend, ReadFrameReturnsData) {
    infrastructure::MockCameraBackend backend;
    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 320;
    config.height = 240;
    config.framerate = 30;

    auto stream = backend.open_stream(config);
    ASSERT_NE(stream, nullptr);

    std::vector<uint8_t> data;
    int64_t pts = 0;
    bool got_frame = stream->read_frame(data, pts);
    EXPECT_TRUE(got_frame);
    EXPECT_FALSE(data.empty());
    EXPECT_EQ(data.size(), 320u * 240u * 3u);
    EXPECT_GE(pts, 0);
}

TEST(MockCameraBackend, ReadFrameHasCorrectPts) {
    infrastructure::MockCameraBackend backend;
    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 160;
    config.height = 120;
    config.framerate = 30;

    auto stream = backend.open_stream(config);
    ASSERT_NE(stream, nullptr);

    std::vector<uint8_t> data;
    int64_t pts1 = 0, pts2 = 0;
    stream->read_frame(data, pts1);
    stream->read_frame(data, pts2);

    EXPECT_GE(pts1, 0);
    EXPECT_GT(pts2, pts1);
}

TEST(MockCameraBackend, FrameDropEveryNth) {
    infrastructure::MockCameraBackend backend;
    backend.set_drop_every_n(3);

    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.width = 160;
    config.height = 120;
    config.framerate = 30;

    auto stream = backend.open_stream(config);
    ASSERT_NE(stream, nullptr);

    int frames_read = 0;
    for (int i = 0; i < 10; i++) {
        std::vector<uint8_t> data;
        int64_t pts;
        if (stream->read_frame(data, pts)) {
            frames_read++;
            EXPECT_EQ(data.size(), 160u * 120u * 3u);
        }
    }

    EXPECT_LT(frames_read, 10);
}

TEST(MockCameraBackend, DisconnectSimulation) {
    infrastructure::MockCameraBackend backend;
    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.width = 160;
    config.height = 120;
    config.framerate = 30;

    auto stream = backend.open_stream(config);
    ASSERT_NE(stream, nullptr);
    EXPECT_TRUE(stream->is_open());

    backend.simulate_disconnect("mock_cam_0");
    EXPECT_FALSE(stream->is_open());
}

TEST(MockCameraBackend, CapabilitiesReturnsValid) {
    infrastructure::MockCameraBackend backend;
    auto caps = backend.get_capabilities();
    EXPECT_FALSE(caps.streams.empty());
    EXPECT_GT(caps.streams[0].max_width, 0);
    EXPECT_GT(caps.streams[0].max_height, 0);
}

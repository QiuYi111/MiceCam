#include <gtest/gtest.h>

#include "domain/PluginRegistry.h"
#include "domain/StreamConfig.h"
#include "infrastructure/CameraManager.h"
#include "infrastructure/MockCameraBackend.h"

using namespace micecam;

TEST(CameraManager, DiscoverAllAggregatesBackends) {
    infrastructure::CameraManager mgr;
    auto mock = std::make_unique<infrastructure::MockCameraBackend>();
    mgr.register_backend(std::move(mock));

    auto results = mgr.discover_all();
    EXPECT_GE(results.size(), 1u);
}

TEST(CameraManager, OpenCameraFindsCorrectBackend) {
    infrastructure::CameraManager mgr;
    auto mock = std::make_unique<infrastructure::MockCameraBackend>();
    mgr.register_backend(std::move(mock));

    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 640;
    config.height = 480;
    config.framerate = 30;

    auto stream = mgr.open_stream(config);
    ASSERT_NE(stream, nullptr);
    EXPECT_TRUE(stream->is_open());
}

TEST(CameraManager, OpenCameraUnknownDeviceReturnsNull) {
    infrastructure::CameraManager mgr;
    auto mock = std::make_unique<infrastructure::MockCameraBackend>();
    mgr.register_backend(std::move(mock));

    domain::StreamConfig config;
    config.device_id = "nonexistent_device";
    config.stream_index = 0;

    auto stream = mgr.open_stream(config);
    EXPECT_EQ(stream, nullptr);
}

TEST(CameraManager, MultipleBackendsRegistered) {
    infrastructure::CameraManager mgr;
    mgr.register_backend(std::make_unique<infrastructure::MockCameraBackend>());
    mgr.register_backend(std::make_unique<infrastructure::MockCameraBackend>());

    auto results = mgr.discover_all();
    EXPECT_GE(results.size(), 1u);
}

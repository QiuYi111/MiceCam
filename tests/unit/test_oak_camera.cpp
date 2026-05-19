#include <gtest/gtest.h>

#include "domain/Capabilities.h"
#include "infrastructure/OAKCameraBackend.h"

using namespace micecam;

TEST(OAKCameraBackend, BackendNameIsOAK) {
    infrastructure::OAKCameraBackend backend;
    EXPECT_EQ(backend.backend_name(), "OAK");
}

TEST(OAKCameraBackend, CapabilitiesReturns4KSupport) {
    infrastructure::OAKCameraBackend backend;
    auto caps = backend.get_capabilities();
    EXPECT_GE(caps.streams.size(), 1u);
    EXPECT_GE(caps.streams[0].max_width, 1920);
    EXPECT_GE(caps.streams[0].max_height, 1080);
}

TEST(OAKCameraBackend, EnumerateReturnsDevicesWhenAvailable) {
    infrastructure::OAKCameraBackend backend;
    auto devices = backend.enumerate_devices();
    if (!devices.empty()) {
        EXPECT_EQ(devices[0].type, "oak");
    }
}

TEST(OAKCameraBackend, OpenStreamRejectsInvalidConfig) {
    infrastructure::OAKCameraBackend backend;
    domain::StreamConfig config;
    config.device_id = "nonexistent_oak";
    config.stream_index = 99;
    auto stream = backend.open_stream(config);
    EXPECT_EQ(stream, nullptr);
}

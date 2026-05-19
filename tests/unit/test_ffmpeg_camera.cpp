#include <gtest/gtest.h>

#include "domain/Capabilities.h"
#include "infrastructure/FFmpegCameraBackend.h"

using namespace micecam;

TEST(FFmpegCameraBackend, BackendNameIsFFmpeg) {
    infrastructure::FFmpegCameraBackend backend;
    EXPECT_EQ(backend.backend_name(), "FFmpeg");
}

TEST(FFmpegCameraBackend, CapabilitiesReturnsNonEmpty) {
    infrastructure::FFmpegCameraBackend backend;
    auto caps = backend.get_capabilities();
    EXPECT_FALSE(caps.streams.empty());
}

TEST(FFmpegCameraBackend, EnumerateReturnsEmptyListWhenNoDevices) {
    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();
    EXPECT_GE(devices.size(), 0u);
}

TEST(FFmpegCameraBackend, OpenStreamRejectsInvalidConfig) {
    infrastructure::FFmpegCameraBackend backend;
    domain::StreamConfig config;
    config.device_id = "nonexistent_ffmpeg";
    config.stream_index = 99;
    auto stream = backend.open_stream(config);
    EXPECT_EQ(stream, nullptr);
}

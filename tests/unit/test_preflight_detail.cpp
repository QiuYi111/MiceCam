#include <gtest/gtest.h>
#include "domain/Capabilities.h"
#include "domain/StreamConfig.h"
#include "pipeline/PreflightValidator.h"

using namespace micecam;

TEST(PreflightDetail, ReportsUnsupportedResolutionAsFieldFailure) {
    pipeline::PreflightValidator validator;
    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 3840;
    config.height = 2160;
    config.framerate = 30;
    config.pixel_format = "rgb24";

    domain::Capabilities caps;
    domain::StreamInfo stream;
    stream.index = 0;
    stream.resolutions = {{1920, 1080, "1920 x 1080"}};
    stream.supported_framerates = {30};
    stream.supported_formats = {"rgb24"};
    caps.streams.push_back(stream);

    auto result = validator.validate_stream_capabilities(config, caps);
    EXPECT_FALSE(result.passed);
    ASSERT_EQ(result.items.size(), 1u);
    EXPECT_EQ(result.items.front().code, "unsupported_resolution");
    EXPECT_EQ(result.items.front().stream_id, "mock_cam_0");
}

TEST(PreflightDetail, PassingCapabilityProducesNoItems) {
    pipeline::PreflightValidator validator;
    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 1920;
    config.height = 1080;
    config.framerate = 30;
    config.pixel_format = "rgb24";

    domain::Capabilities caps;
    domain::StreamInfo stream;
    stream.index = 0;
    stream.resolutions = {{1920, 1080, "1920 x 1080"}};
    stream.supported_framerates = {15, 30, 60};
    stream.supported_formats = {"rgb24"};
    caps.streams.push_back(stream);

    auto result = validator.validate_stream_capabilities(config, caps);
    EXPECT_TRUE(result.passed);
    EXPECT_TRUE(result.items.empty());
}

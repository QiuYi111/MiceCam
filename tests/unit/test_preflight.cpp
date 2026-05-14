#include <gtest/gtest.h>

#include <sys/statvfs.h>

#include "domain/DeviceInfo.h"
#include "domain/StreamConfig.h"
#include "pipeline/PreflightValidator.h"

using namespace micecam;

TEST(PreflightValidator, DiskSpaceCheckPasses) {
    pipeline::PreflightValidator validator;
    struct statvfs buf;
    if (statvfs("/tmp", &buf) == 0) {
        uint64_t avail = static_cast<uint64_t>(buf.f_bavail) * buf.f_frsize;
        EXPECT_TRUE(validator.check_disk_space("/tmp", avail / 10));
    }
}

TEST(PreflightValidator, DiskSpaceCheckFails) {
    pipeline::PreflightValidator validator;
    bool result = validator.check_disk_space("/tmp", UINT64_MAX);
    EXPECT_FALSE(result);
}

TEST(PreflightValidator, CapabilityCheckMatches) {
    pipeline::PreflightValidator validator;
    domain::StreamConfig config;
    config.width = 1920;
    config.height = 1080;
    config.framerate = 30;
    config.pixel_format = "nv12";

    domain::Capabilities caps;
    domain::StreamInfo si;
    si.max_width = 4096;
    si.max_height = 2160;
    si.supported_formats = {"nv12", "yuv420p"};
    si.supported_framerates = {30, 60};
    caps.streams.push_back(si);

    EXPECT_TRUE(validator.check_capabilities(config, caps));
}

TEST(PreflightValidator, CapabilityCheckResolutionTooHigh) {
    pipeline::PreflightValidator validator;
    domain::StreamConfig config;
    config.width = 8192;
    config.height = 4320;

    domain::Capabilities caps;
    domain::StreamInfo si;
    si.max_width = 4096;
    si.max_height = 2160;
    si.supported_formats = {"nv12"};
    si.supported_framerates = {30};
    caps.streams.push_back(si);

    EXPECT_FALSE(validator.check_capabilities(config, caps));
}

TEST(PreflightValidator, CapabilityCheckUnsupportedFormat) {
    pipeline::PreflightValidator validator;
    domain::StreamConfig config;
    config.width = 1920;
    config.height = 1080;
    config.pixel_format = "rgb24";

    domain::Capabilities caps;
    domain::StreamInfo si;
    si.max_width = 4096;
    si.max_height = 2160;
    si.supported_formats = {"nv12"};
    si.supported_framerates = {30};
    caps.streams.push_back(si);

    EXPECT_FALSE(validator.check_capabilities(config, caps));
}

TEST(PreflightValidator, FullValidationPassesWhenDiskHasSpace) {
    pipeline::PreflightValidator validator;
    std::vector<domain::StreamConfig> configs;
    domain::StreamConfig config;
    config.width = 1920;
    config.height = 1080;
    config.framerate = 30;
    config.pixel_format = "nv12";
    configs.push_back(config);

    auto result = validator.validate(configs, "/tmp", 60);
    EXPECT_TRUE(result.passed);
}

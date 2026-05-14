#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

#include "domain/DeviceInfo.h"
#include "domain/StreamConfig.h"
#include "pipeline/PreflightValidator.h"

using namespace micecam;

TEST(PreflightValidator, DiskSpaceCheckPasses) {
    pipeline::PreflightValidator validator;
#ifdef _WIN32
    ULARGE_INTEGER free_bytes;
    bool has_space = GetDiskFreeSpaceExA("C:\\", &free_bytes, nullptr, nullptr) != 0;
#else
    struct statvfs buf;
    bool has_space = statvfs("/tmp", &buf) == 0;
#endif
    if (has_space) {
#ifdef _WIN32
        bool result = validator.check_disk_space("C:\\", 1000);
#else
        bool result = validator.check_disk_space("/tmp", 1000);
#endif
        EXPECT_TRUE(result);
    }
}

TEST(PreflightValidator, DiskSpaceCheckFails) {
    pipeline::PreflightValidator validator;
#ifdef _WIN32
    bool result = validator.check_disk_space("C:\\", UINT64_MAX);
#else
    bool result = validator.check_disk_space("/tmp", UINT64_MAX);
#endif
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

    auto result = validator.validate(configs,
#ifdef _WIN32
        "C:\\",
#else
        "/tmp",
#endif
        60);
    EXPECT_TRUE(result.passed);
}

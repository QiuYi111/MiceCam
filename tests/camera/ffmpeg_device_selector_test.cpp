#include "infrastructure/ffmpeg_device_selector.h"

#include <gtest/gtest.h>

namespace {

TEST(FFmpegDeviceSelectorTest, PrefixesWindowsVideoDevicesWhenNeeded) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {"Integrated Camera", "USB Camera"},
        1,
        "video="
    );

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, "video=USB Camera");
}

TEST(FFmpegDeviceSelectorTest, PreservesExplicitVideoPrefix) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {"video=Integrated Camera"},
        0,
        "video="
    );

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, "video=Integrated Camera");
}

TEST(FFmpegDeviceSelectorTest, RejectsOutOfRangeDeviceIndex) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {"Integrated Camera"},
        2,
        "video="
    );

    EXPECT_FALSE(resolved.has_value());
}

}  // namespace

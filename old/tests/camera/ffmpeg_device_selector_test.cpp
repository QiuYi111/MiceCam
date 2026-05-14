#include "infrastructure/ffmpeg_device_selector.h"

#include <gtest/gtest.h>

namespace {

TEST(FFmpegDeviceSelectorTest, PrefixesWindowsVideoDevicesWhenNeeded) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {"Integrated Camera", "USB Camera"},
        1,
        "video=",
        std::nullopt
    );

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, "video=USB Camera");
}

TEST(FFmpegDeviceSelectorTest, PreservesExplicitVideoPrefix) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {"video=Integrated Camera"},
        0,
        "video=",
        std::nullopt
    );

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, "video=Integrated Camera");
}

TEST(FFmpegDeviceSelectorTest, RejectsOutOfRangeDeviceIndex) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {"Integrated Camera"},
        2,
        "video=",
        std::nullopt
    );

    EXPECT_FALSE(resolved.has_value());
}

TEST(FFmpegDeviceSelectorTest, RejectsBlankDeviceNamesInsteadOfSynthesizingEmptyPrefixes) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {""},
        0,
        "video=",
        std::nullopt
    );

    EXPECT_FALSE(resolved.has_value());
}

TEST(FFmpegDeviceSelectorTest, PrefersExplicitFriendlyNameWhenProvided) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {"Integrated Camera"},
        0,
        "video=",
        std::string("HD USB Camera")
    );

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, "video=HD USB Camera");
}

TEST(FFmpegDeviceSelectorTest, RejectsBlankFriendlyNameWhenInventoryIsUnavailable) {
    const auto resolved = micecam::resolve_ffmpeg_input_device_name(
        {},
        0,
        "video=",
        std::string("   ")
    );

    EXPECT_FALSE(resolved.has_value());
}

}  // namespace

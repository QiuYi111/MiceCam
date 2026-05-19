#include <gtest/gtest.h>

#include "infrastructure/HardwareEncoderSelector.h"

using namespace micecam::infrastructure;

TEST(HardwareEncoderSelector, DetectReturnsNonEmpty) {
    auto name = HardwareEncoderSelector::detect_platform_encoder();
    EXPECT_FALSE(name.empty());
}

TEST(HardwareEncoderSelector, IsHardwareEncoderForKnownHW) {
    EXPECT_TRUE(HardwareEncoderSelector::is_hardware_encoder("h264_videotoolbox"));
    EXPECT_TRUE(HardwareEncoderSelector::is_hardware_encoder("h264_nvenc"));
    EXPECT_TRUE(HardwareEncoderSelector::is_hardware_encoder("h264_qsv"));
    EXPECT_TRUE(HardwareEncoderSelector::is_hardware_encoder("h264_vaapi"));
    EXPECT_TRUE(HardwareEncoderSelector::is_hardware_encoder("h264_amf"));
}

TEST(HardwareEncoderSelector, IsHardwareEncoderFalseForSoftware) {
    EXPECT_FALSE(HardwareEncoderSelector::is_hardware_encoder("libx264"));
    EXPECT_FALSE(HardwareEncoderSelector::is_hardware_encoder(""));
    EXPECT_FALSE(HardwareEncoderSelector::is_hardware_encoder("h264"));
}

TEST(HardwareEncoderSelector, FallbackIsLibx264) {
    EXPECT_EQ(HardwareEncoderSelector::get_fallback_encoder(), "libx264");
}

#ifdef __APPLE__
TEST(HardwareEncoderSelector, MacOSDetectsVideoToolbox) {
    auto name = HardwareEncoderSelector::detect_platform_encoder();
    EXPECT_EQ(name, "h264_videotoolbox");
}
#endif

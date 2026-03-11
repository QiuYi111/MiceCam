#include "infrastructure/oak_device_selector.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

TEST(OakDeviceSelectorTest, ReturnsSelectedDeviceWhenIndexIsValid) {
    const auto selected = micecam::resolve_oak_device_info<std::string>(
        {"oak-a", "oak-b"},
        1
    );

    ASSERT_TRUE(selected.has_value());
    EXPECT_EQ(*selected, "oak-b");
}

TEST(OakDeviceSelectorTest, RejectsNegativeDeviceIndex) {
    const auto selected = micecam::resolve_oak_device_info<std::string>(
        {"oak-a"},
        -1
    );

    EXPECT_FALSE(selected.has_value());
}

TEST(OakDeviceSelectorTest, RejectsOutOfRangeDeviceIndex) {
    const auto selected = micecam::resolve_oak_device_info<std::string>(
        {"oak-a"},
        3
    );

    EXPECT_FALSE(selected.has_value());
}

}  // namespace

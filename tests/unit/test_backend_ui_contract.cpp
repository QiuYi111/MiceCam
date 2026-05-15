#include <gtest/gtest.h>

#include "domain/DeviceInfo.h"
#include "infrastructure/MockCameraBackend.h"

using namespace micecam;

TEST(BackendUiContract, MockDiscoveryProvidesUiReadyCameraRows) {
    infrastructure::MockCameraBackend backend;
    auto devices = backend.enumerate_devices();
    ASSERT_GE(devices.size(), 1u);
    const auto& device = devices.front();
    EXPECT_FALSE(device.id.empty());
    EXPECT_FALSE(device.name.empty());
    EXPECT_FALSE(device.type.empty());
    ASSERT_GE(device.streams.size(), 1u);
    const auto& stream = device.streams.front();
    EXPECT_FALSE(stream.label.empty());
    EXPECT_GE(stream.resolutions.size(), 1u);
    EXPECT_GE(stream.supported_framerates.size(), 1u);
    EXPECT_GE(stream.supported_formats.size(), 1u);
    EXPECT_TRUE(stream.available);
}

TEST(BackendUiContract, MockCapabilitiesArePerStreamAndSelectable) {
    infrastructure::MockCameraBackend backend;
    auto devices = backend.enumerate_devices();
    ASSERT_GE(devices.size(), 1u);
    ASSERT_GE(devices.front().streams.size(), 1u);
    auto caps = backend.get_capabilities(devices.front().id, devices.front().streams.front().index);
    ASSERT_GE(caps.streams.size(), 1u);
    const auto& stream = caps.streams.front();
    EXPECT_GE(stream.resolutions.size(), 2u);
    EXPECT_GE(stream.supported_framerates.size(), 2u);
    EXPECT_GE(stream.supported_formats.size(), 1u);
    EXPECT_FALSE(caps.encoder_name.empty());
}

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "domain/EncoderConfig.h"
#include "infrastructure/FFmpegEncoder.h"

using namespace micecam;

namespace {

std::vector<uint8_t> generate_test_rgb(int width, int height) {
    std::vector<uint8_t> frame(width * height * 3);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 3;
            frame[idx] = static_cast<uint8_t>((x * 255) / width);
            frame[idx + 1] = static_cast<uint8_t>((y * 255) / height);
            frame[idx + 2] = 128;
        }
    }
    return frame;
}

} // namespace

TEST(FFmpegEncoder, InitializeWithDefaultConfig) {
    infrastructure::FFmpegEncoder encoder;
    domain::EncoderConfig config;
    config.bitrate_kbps = 5000;
    config.keyframe_interval = 60;
    config.crf = 23;
    config.max_b_frames = 0;
    config.prefer_hardware = false;

    EXPECT_TRUE(encoder.initialize(config));
}

TEST(FFmpegEncoder, EncoderNameReturnsNonEmptyAfterInit) {
    infrastructure::FFmpegEncoder encoder;
    domain::EncoderConfig config;
    config.prefer_hardware = false;

    ASSERT_TRUE(encoder.initialize(config));
    EXPECT_FALSE(encoder.encoder_name().empty());
}

TEST(FFmpegEncoder, EncodeProducesOutput) {
    infrastructure::FFmpegEncoder encoder;
    domain::EncoderConfig config;
    config.prefer_hardware = false;

    ASSERT_TRUE(encoder.initialize(config));

    const int width = 320;
    const int height = 240;
    auto rgb = generate_test_rgb(width, height);

    auto packet = encoder.encode(rgb.data(), width, height, 0);
    EXPECT_FALSE(packet.empty());
}

TEST(FFmpegEncoder, Encode30Frames) {
    infrastructure::FFmpegEncoder encoder;
    domain::EncoderConfig config;
    config.prefer_hardware = false;

    ASSERT_TRUE(encoder.initialize(config));

    const int width = 320;
    const int height = 240;
    auto rgb = generate_test_rgb(width, height);

    int encoded_count = 0;
    for (int i = 0; i < 30; i++) {
        auto packet = encoder.encode(rgb.data(), width, height, i);
        if (!packet.empty()) {
            encoded_count++;
        }
    }

    std::vector<uint8_t> flushed;
    encoder.flush(flushed);

    EXPECT_GT(encoded_count, 0);
}

TEST(FFmpegEncoder, FlushWorks) {
    infrastructure::FFmpegEncoder encoder;
    domain::EncoderConfig config;
    config.prefer_hardware = false;

    ASSERT_TRUE(encoder.initialize(config));

    const int width = 320;
    const int height = 240;
    auto rgb = generate_test_rgb(width, height);

    encoder.encode(rgb.data(), width, height, 0);

    std::vector<uint8_t> flushed;
    EXPECT_TRUE(encoder.flush(flushed));
}

TEST(FFmpegEncoder, EncoderNameIsCorrect) {
    infrastructure::FFmpegEncoder encoder;
    domain::EncoderConfig config;
    config.prefer_hardware = false;

    ASSERT_TRUE(encoder.initialize(config));
    std::string name = encoder.encoder_name();
    EXPECT_FALSE(name.empty());
    EXPECT_NE(name, "unknown");
}

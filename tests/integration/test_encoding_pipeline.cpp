#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "domain/EncoderConfig.h"
#include "infrastructure/FFmpegEncoder.h"
#include "infrastructure/StreamWriter.h"

using namespace micecam;

namespace {

const char* TEST_OUTPUT = "test_output";

std::vector<uint8_t> generate_synthetic_rgb(int width, int height) {
    std::vector<uint8_t> frame(width * height * 3);
    const int bar_width = width / 7;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 3;
            int bar = std::min(x / bar_width, 6);
            const uint8_t bars_r[7] = {255, 255, 255, 0, 0, 0, 0};
            const uint8_t bars_g[7] = {255, 255, 0, 255, 255, 0, 0};
            const uint8_t bars_b[7] = {255, 0, 255, 255, 0, 255, 0};

            frame[idx] = bars_r[bar];
            frame[idx + 1] = bars_g[bar];
            frame[idx + 2] = bars_b[bar];
        }
    }
    return frame;
}

bool probe_valid_h264(const std::string& path) {
    AVFormatContext* ctx = nullptr;
    int ret = avformat_open_input(&ctx, path.c_str(), nullptr, nullptr);
    if (ret < 0) return false;

    avformat_find_stream_info(ctx, nullptr);

    bool has_h264 = false;
    for (unsigned i = 0; i < ctx->nb_streams; i++) {
        if (ctx->streams[i]->codecpar->codec_id == AV_CODEC_ID_H264) {
            has_h264 = true;
            break;
        }
    }

    bool valid = has_h264 && ctx->nb_streams > 0;
    avformat_close_input(&ctx);
    return valid;
}

} // namespace

TEST(EncodingPipeline, FullEncodeChainProducesValidMP4) {
    const int W = 320;
    const int H = 240;
    const int FPS = 30;
    const int FRAMES = 30;
    const std::string path = std::string(TEST_OUTPUT) + "/integration_output.mp4";

    auto rgb = generate_synthetic_rgb(W, H);

    domain::EncoderConfig config;
    config.bitrate_kbps = 2000;
    config.keyframe_interval = 30;
    config.max_b_frames = 0;
    config.prefer_hardware = false;

    infrastructure::FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(config));
    std::string enc_name = encoder.encoder_name();
    EXPECT_FALSE(enc_name.empty());

    infrastructure::StreamWriter writer;
    ASSERT_TRUE(writer.open(path, W, H, FPS));

    int64_t pts = 0;
    for (int i = 0; i < FRAMES; i++) {
        auto packet = encoder.encode(rgb.data(), W, H, pts);
        if (!packet.empty()) {
            bool keyframe = (i == 0);
            ASSERT_TRUE(writer.write_packet(packet.data(), packet.size(), pts, pts, keyframe));
        }
        pts++;
    }

    std::vector<uint8_t> flushed;
    encoder.flush(flushed);
    if (!flushed.empty()) {
        writer.write_packet(flushed.data(), flushed.size(), pts, pts, true);
    }

    writer.close();

    FILE* f = fopen(path.c_str(), "rb");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fclose(f);
    EXPECT_GT(file_size, 0);

    bool is_valid = probe_valid_h264(path);
    EXPECT_TRUE(is_valid) << "ffprobe did not find valid H264 in " << path;
}

TEST(EncodingPipeline, EncodeWithFlushProducesOutput) {
    const int W = 160;
    const int H = 120;
    const int FPS = 30;
    const std::string path = std::string(TEST_OUTPUT) + "/integration_flush.mp4";

    auto rgb = generate_synthetic_rgb(W, H);

    domain::EncoderConfig config;
    config.bitrate_kbps = 1000;
    config.keyframe_interval = 15;
    config.max_b_frames = 0;
    config.prefer_hardware = false;

    infrastructure::FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(config));

    infrastructure::StreamWriter writer;
    ASSERT_TRUE(writer.open(path, W, H, FPS));

    for (int i = 0; i < 10; i++) {
        auto packet = encoder.encode(rgb.data(), W, H, i);
        if (!packet.empty()) {
            writer.write_packet(packet.data(), packet.size(), i, i, (i == 0));
        }
    }

    std::vector<uint8_t> flushed;
    encoder.flush(flushed);

    writer.close();

    FILE* f = fopen(path.c_str(), "rb");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fclose(f);
    EXPECT_GT(file_size, 0);
}

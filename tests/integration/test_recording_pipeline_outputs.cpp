#include <gtest/gtest.h>
#include <filesystem>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "domain/EncoderConfig.h"
#include "domain/StreamConfig.h"
#include "pipeline/RecordingPipeline.h"

namespace {

std::vector<uint8_t> rgb_frame(int width, int height, int seed) {
    std::vector<uint8_t> data(static_cast<size_t>(width) * height * 3);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
            data[idx]     = static_cast<uint8_t>((x + seed) % 256);
            data[idx + 1] = static_cast<uint8_t>((y + seed) % 256);
            data[idx + 2] = static_cast<uint8_t>((x + y + seed) % 256);
        }
    }
    return data;
}

bool has_h264_video_stream(const std::string& path) {
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, path.c_str(), nullptr, nullptr) < 0) return false;
    bool found = false;
    if (avformat_find_stream_info(ctx, nullptr) >= 0) {
        for (unsigned i = 0; i < ctx->nb_streams; ++i) {
            if (ctx->streams[i]->codecpar->codec_id == AV_CODEC_ID_H264) {
                found = true; break;
            }
        }
    }
    avformat_close_input(&ctx);
    return found;
}

} // namespace

TEST(RecordingPipelineOutputs, RawRgbFramesProduceValidH264Mp4) {
    const std::string root = "/tmp/micecam_recording_pipeline_outputs";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    micecam::pipeline::SessionConfig config;
    config.session_id = "valid_h264";
    config.output_dir = root;
    config.encoder.prefer_hardware = false;
    config.encoder.bitrate_kbps = 1000;
    config.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream;
    stream.device_id = "mock_cam_0";
    stream.stream_index = 0;
    stream.width = 160;
    stream.height = 120;
    stream.framerate = 30;
    stream.pixel_format = "rgb24";
    config.streams.push_back(stream);

    micecam::pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    for (int i = 0; i < 30; ++i) {
        const auto frame = rgb_frame(stream.width, stream.height, i);
        micecam::pipeline::FrameData data;
        data.stream_id = "mock_cam_0_0";
        data.data = frame.data();
        data.size = frame.size();
        data.width = stream.width;
        data.height = stream.height;
        data.pts = i;
        data.source_format = "rgb24";
        ASSERT_TRUE(pipeline.push_frame(data));
    }

    pipeline.stop();
    const auto [meta, stats] = pipeline.result();

    const std::string mp4_path = root + "/valid_h264/mock_cam_0_0.mp4";
    EXPECT_TRUE(std::filesystem::exists(mp4_path));
    EXPECT_TRUE(has_h264_video_stream(mp4_path));
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats.front().frames_actual, 30u);
    EXPECT_GT(stats.front().bytes_written, 0u);
    EXPECT_FALSE(stats.front().encoder_used.empty());
}

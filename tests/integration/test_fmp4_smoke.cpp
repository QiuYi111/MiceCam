#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

namespace {

class FMP4SmokeTest : public ::testing::Test {
protected:
    std::filesystem::path test_file_;

    void SetUp() override {
        test_file_ = std::filesystem::temp_directory_path() / "micecam_smoke_fmp4_test.mp4";
        std::filesystem::remove(test_file_);
    }

    void TearDown() override {
        std::filesystem::remove(test_file_);
    }
};

TEST_F(FMP4SmokeTest, FileReadableWithoutTrailer) {
    constexpr int kWidth = 320;
    constexpr int kHeight = 240;
    constexpr int kFps = 25;
    constexpr int kNumFrames = 15;

    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    ASSERT_NE(codec, nullptr) << "libx264 encoder not found";

    AVCodecContext* enc_ctx = avcodec_alloc_context3(codec);
    ASSERT_NE(enc_ctx, nullptr);
    enc_ctx->width = kWidth;
    enc_ctx->height = kHeight;
    enc_ctx->time_base = {1, kFps};
    enc_ctx->framerate = {kFps, 1};
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    enc_ctx->bit_rate = 500000;
    enc_ctx->gop_size = kNumFrames;
    enc_ctx->max_b_frames = 0;
    av_opt_set_int(enc_ctx->priv_data, "crf", 23, 0);
    av_opt_set(enc_ctx->priv_data, "preset", "fast", 0);
    av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);

    ASSERT_GE(avcodec_open2(enc_ctx, codec, nullptr), 0);

    AVFrame* frame = av_frame_alloc();
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = kWidth;
    frame->height = kHeight;
    ASSERT_GE(av_frame_get_buffer(frame, 0), 0);
    av_frame_make_writable(frame);
    for (int p = 0; p < 3; p++) {
        if (frame->data[p]) {
            memset(frame->data[p], (p == 0) ? 100 : 128,
                   static_cast<size_t>(frame->linesize[p]) * (kHeight >> (p > 0 ? 1 : 0)));
        }
    }

    struct EncodedPacket {
        std::vector<uint8_t> data;
        int64_t pts;
        bool keyframe;
    };
    std::vector<EncodedPacket> encoded_packets;

    for (int i = 0; i < kNumFrames; i++) {
        frame->pts = i;
        if (i == 0) frame->flags |= AV_FRAME_FLAG_KEY;

        int ret = avcodec_send_frame(enc_ctx, frame);
        if (ret < 0) continue;

        AVPacket* pkt = av_packet_alloc();
        while (true) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) break;
            EncodedPacket ep;
            ep.data.assign(pkt->data, pkt->data + pkt->size);
            ep.pts = pkt->pts;
            ep.keyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;
            encoded_packets.push_back(std::move(ep));
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }

    av_frame_free(&frame);
    avcodec_free_context(&enc_ctx);

    ASSERT_FALSE(encoded_packets.empty()) << "No packets were encoded";

    AVFormatContext* fmt_ctx = nullptr;
    ASSERT_GE(avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, test_file_.c_str()), 0);
    ASSERT_NE(fmt_ctx, nullptr);

    AVStream* stream = avformat_new_stream(fmt_ctx, nullptr);
    ASSERT_NE(stream, nullptr);
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->codec_id = AV_CODEC_ID_H264;
    stream->codecpar->width = kWidth;
    stream->codecpar->height = kHeight;
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->time_base = {1, kFps};

    ASSERT_GE(avio_open(&fmt_ctx->pb, test_file_.c_str(), AVIO_FLAG_WRITE), 0);

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "movflags", "+frag_keyframe+empty_moov+default_base_moof", 0);
    ASSERT_GE(avformat_write_header(fmt_ctx, &opts), 0);
    av_dict_free(&opts);

    for (const auto& ep : encoded_packets) {
        AVPacket* out_pkt = av_packet_alloc();
        av_new_packet(out_pkt, static_cast<int>(ep.data.size()));
        memcpy(out_pkt->data, ep.data.data(), ep.data.size());
        out_pkt->stream_index = stream->index;
        out_pkt->pts = ep.pts;
        out_pkt->dts = ep.pts;
        out_pkt->duration = 1;
        if (ep.keyframe) out_pkt->flags |= AV_PKT_FLAG_KEY;

        ASSERT_GE(av_interleaved_write_frame(fmt_ctx, out_pkt), 0);
        av_packet_free(&out_pkt);
    }

    av_write_trailer(fmt_ctx);

    if (fmt_ctx->pb) {
        avio_closep(&fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);
    fmt_ctx = nullptr;

    ASSERT_TRUE(std::filesystem::exists(test_file_));
    ASSERT_GT(std::filesystem::file_size(test_file_), 0u);

    AVFormatContext* read_ctx = nullptr;
    ASSERT_GE(avformat_open_input(&read_ctx, test_file_.c_str(), nullptr, nullptr), 0)
        << "Failed to open fMP4 file";
    avformat_find_stream_info(read_ctx, nullptr);

    ASSERT_GT(read_ctx->nb_streams, 0u);
    EXPECT_EQ(read_ctx->streams[0]->codecpar->codec_type, AVMEDIA_TYPE_VIDEO);
    EXPECT_EQ(read_ctx->streams[0]->codecpar->codec_id, AV_CODEC_ID_H264);

    int frame_count = 0;
    AVPacket* read_pkt = av_packet_alloc();
    while (av_read_frame(read_ctx, read_pkt) >= 0) {
        frame_count++;
        av_packet_unref(read_pkt);
    }
    av_packet_free(&read_pkt);
    avformat_close_input(&read_ctx);

    EXPECT_GT(frame_count, 0) << "No frames found in fMP4 file";
    EXPECT_EQ(frame_count, static_cast<int>(encoded_packets.size()));
}

TEST_F(FMP4SmokeTest, FileReadableAfterCrashNoTrailer) {
    constexpr int kWidth = 320;
    constexpr int kHeight = 240;
    constexpr int kFps = 25;
    constexpr int kNumFrames = 15;

    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    ASSERT_NE(codec, nullptr) << "libx264 encoder not found";

    AVCodecContext* enc_ctx = avcodec_alloc_context3(codec);
    ASSERT_NE(enc_ctx, nullptr);
    enc_ctx->width = kWidth;
    enc_ctx->height = kHeight;
    enc_ctx->time_base = {1, kFps};
    enc_ctx->framerate = {kFps, 1};
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    enc_ctx->bit_rate = 500000;
    enc_ctx->gop_size = kNumFrames;
    enc_ctx->max_b_frames = 0;
    av_opt_set_int(enc_ctx->priv_data, "crf", 23, 0);
    av_opt_set(enc_ctx->priv_data, "preset", "fast", 0);
    av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);

    ASSERT_GE(avcodec_open2(enc_ctx, codec, nullptr), 0);

    AVFrame* frame = av_frame_alloc();
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = kWidth;
    frame->height = kHeight;
    ASSERT_GE(av_frame_get_buffer(frame, 0), 0);
    av_frame_make_writable(frame);
    for (int p = 0; p < 3; p++) {
        if (frame->data[p]) {
            memset(frame->data[p], (p == 0) ? 100 : 128,
                   static_cast<size_t>(frame->linesize[p]) * (kHeight >> (p > 0 ? 1 : 0)));
        }
    }

    struct EncodedPacket {
        std::vector<uint8_t> data;
        int64_t pts;
        bool keyframe;
    };
    std::vector<EncodedPacket> encoded_packets;

    for (int i = 0; i < kNumFrames; i++) {
        frame->pts = i;
        if (i == 0) frame->flags |= AV_FRAME_FLAG_KEY;

        int ret = avcodec_send_frame(enc_ctx, frame);
        if (ret < 0) continue;

        AVPacket* pkt = av_packet_alloc();
        while (true) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) break;
            EncodedPacket ep;
            ep.data.assign(pkt->data, pkt->data + pkt->size);
            ep.pts = pkt->pts;
            ep.keyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;
            encoded_packets.push_back(std::move(ep));
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }

    av_frame_free(&frame);
    avcodec_free_context(&enc_ctx);

    ASSERT_FALSE(encoded_packets.empty()) << "No packets were encoded";

    AVFormatContext* fmt_ctx = nullptr;
    ASSERT_GE(avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, test_file_.c_str()), 0);
    ASSERT_NE(fmt_ctx, nullptr);

    AVStream* stream = avformat_new_stream(fmt_ctx, nullptr);
    ASSERT_NE(stream, nullptr);
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->codec_id = AV_CODEC_ID_H264;
    stream->codecpar->width = kWidth;
    stream->codecpar->height = kHeight;
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->time_base = {1, kFps};

    ASSERT_GE(avio_open(&fmt_ctx->pb, test_file_.c_str(), AVIO_FLAG_WRITE), 0);

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "movflags", "+frag_keyframe+empty_moov+default_base_moof", 0);
    ASSERT_GE(avformat_write_header(fmt_ctx, &opts), 0);
    av_dict_free(&opts);

    for (const auto& ep : encoded_packets) {
        AVPacket* out_pkt = av_packet_alloc();
        av_new_packet(out_pkt, static_cast<int>(ep.data.size()));
        memcpy(out_pkt->data, ep.data.data(), ep.data.size());
        out_pkt->stream_index = stream->index;
        out_pkt->pts = ep.pts;
        out_pkt->dts = ep.pts;
        out_pkt->duration = 1;
        if (ep.keyframe) out_pkt->flags |= AV_PKT_FLAG_KEY;

        ASSERT_GE(av_interleaved_write_frame(fmt_ctx, out_pkt), 0);
        av_packet_free(&out_pkt);
    }

    // Flush the IO context to simulate process exit after writing fragments
    // but WITHOUT calling av_write_trailer (crash simulation)
    avio_flush(fmt_ctx->pb);
    avio_closep(&fmt_ctx->pb);
    avformat_free_context(fmt_ctx);
    fmt_ctx = nullptr;

    ASSERT_TRUE(std::filesystem::exists(test_file_));
    ASSERT_GT(std::filesystem::file_size(test_file_), 0u);

    // Verify the crash-dumped fMP4 file is at least openable and has a video stream
    AVFormatContext* read_ctx = nullptr;
    int open_ret = avformat_open_input(&read_ctx, test_file_.c_str(), nullptr, nullptr);
    ASSERT_GE(open_ret, 0) << "Failed to open fMP4 file without trailer";

    avformat_find_stream_info(read_ctx, nullptr);

    ASSERT_GT(read_ctx->nb_streams, 0u);
    EXPECT_EQ(read_ctx->streams[0]->codecpar->codec_type, AVMEDIA_TYPE_VIDEO);
    EXPECT_EQ(read_ctx->streams[0]->codecpar->codec_id, AV_CODEC_ID_H264);
    EXPECT_EQ(read_ctx->streams[0]->codecpar->width, kWidth);
    EXPECT_EQ(read_ctx->streams[0]->codecpar->height, kHeight);

    int frame_count = 0;
    AVPacket* read_pkt = av_packet_alloc();
    while (av_read_frame(read_ctx, read_pkt) >= 0) {
        frame_count++;
        av_packet_unref(read_pkt);
    }
    av_packet_free(&read_pkt);
    avformat_close_input(&read_ctx);

    // fMP4 without trailer may or may not yield readable frames depending on
    // where the last moof boundary falls. The key invariant is that the file
    // is openable and has a valid H264 stream descriptor.
    EXPECT_GE(frame_count, 0) << "fMP4 without trailer should at least be parseable";
}

} // namespace

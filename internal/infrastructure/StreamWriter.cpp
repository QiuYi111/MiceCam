#include "infrastructure/StreamWriter.h"

#include <cstring>
#include <spdlog/spdlog.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace micecam::infrastructure {

StreamWriter::StreamWriter() = default;

StreamWriter::~StreamWriter() {
    if (fmt_ctx_) {
        if (fmt_ctx_->pb) {
            avio_closep(&fmt_ctx_->pb);
        }
        avformat_free_context(fmt_ctx_);
        fmt_ctx_ = nullptr;
    }
}

bool StreamWriter::open(const std::string& path, int width, int height, int fps) {
    std::lock_guard<std::mutex> lock(mutex_);

    AVFormatContext* ctx = nullptr;
    int ret = avformat_alloc_output_context2(&ctx, nullptr, nullptr, path.c_str());
    if (ret < 0 || !ctx) {
        spdlog::error("Failed to alloc output context for {}", path);
        return false;
    }

    AVStream* stream = avformat_new_stream(ctx, nullptr);
    if (!stream) {
        spdlog::error("Failed to create stream");
        avformat_free_context(ctx);
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        spdlog::error("H264 codec not found for stream parameters");
        avformat_free_context(ctx);
        return false;
    }

    stream->id = ctx->nb_streams - 1;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->codec_id = AV_CODEC_ID_H264;
    stream->codecpar->width = width;
    stream->codecpar->height = height;
    stream->time_base = {1, fps};

    ret = avio_open(&ctx->pb, path.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        spdlog::error("Failed to open output file: {}", path);
        avformat_free_context(ctx);
        return false;
    }

    ret = avformat_write_header(ctx, nullptr);
    if (ret < 0) {
        spdlog::error("Failed to write header");
        avio_closep(&ctx->pb);
        avformat_free_context(ctx);
        return false;
    }

    fmt_ctx_ = ctx;
    stream_ = stream;
    packet_index_ = 0;

    spdlog::info("StreamWriter opened: {} ({}x{} @{}fps)", path, width, height, fps);
    return true;
}

bool StreamWriter::write_packet(const uint8_t* data, size_t size, int64_t pts, int64_t dts, bool keyframe) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!fmt_ctx_ || !stream_) return false;

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return false;

    int alloc_ret = av_new_packet(pkt, static_cast<int>(size));
    if (alloc_ret < 0) {
        av_packet_free(&pkt);
        return false;
    }

    memcpy(pkt->data, data, size);
    pkt->stream_index = stream_->index;
    pkt->pts = pts;
    pkt->dts = dts;
    pkt->duration = 1;

    if (keyframe) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }

    av_packet_rescale_ts(pkt, stream_->time_base, stream_->time_base);

    int ret = av_interleaved_write_frame(fmt_ctx_, pkt);
    av_packet_free(&pkt);

    if (ret < 0) {
        spdlog::warn("av_interleaved_write_frame failed: {}", ret);
        return false;
    }

    packet_index_++;
    return true;
}

bool StreamWriter::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!fmt_ctx_) return false;

    av_write_trailer(fmt_ctx_);

    if (fmt_ctx_->pb) {
        avio_closep(&fmt_ctx_->pb);
    }

    avformat_free_context(fmt_ctx_);
    fmt_ctx_ = nullptr;
    stream_ = nullptr;

    spdlog::info("StreamWriter closed: {} packets written", packet_index_);
    return true;
}

} // namespace micecam::infrastructure

#include "infrastructure/FFmpegEncoder.h"

#include <spdlog/spdlog.h>

#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace micecam::infrastructure {

FFmpegEncoder::FFmpegEncoder() = default;

FFmpegEncoder::~FFmpegEncoder() {
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }
    if (enc_ctx_) {
        avcodec_free_context(&enc_ctx_);
    }
}

static const AVCodec* find_h264_encoder(bool prefer_hardware, std::string& out_name) {
    const AVCodec* codec = nullptr;

    if (prefer_hardware) {
#ifdef __APPLE__
        codec = avcodec_find_encoder_by_name("h264_videotoolbox");
        if (codec) { out_name = "h264_videotoolbox"; return codec; }
#endif
        codec = avcodec_find_encoder_by_name("h264_nvenc");
        if (codec) { out_name = "h264_nvenc"; return codec; }
        codec = avcodec_find_encoder_by_name("h264_qsv");
        if (codec) { out_name = "h264_qsv"; return codec; }
        codec = avcodec_find_encoder_by_name("h264_vaapi");
        if (codec) { out_name = "h264_vaapi"; return codec; }
        codec = avcodec_find_encoder_by_name("h264_amf");
        if (codec) { out_name = "h264_amf"; return codec; }
    }

    codec = avcodec_find_encoder_by_name("libx264");
    if (codec) { out_name = "libx264"; return codec; }

    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (codec) { out_name = codec->name; return codec; }

    return nullptr;
}

static AVCodecContext* open_codec_context(const AVCodec* codec, int width, int height, int fps,
                                           int bitrate_kbps, int gop_size, int crf, int max_b_frames,
                                           const std::string& name) {
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) return nullptr;

    ctx->width = width;
    ctx->height = height;
    ctx->time_base = {1, fps};
    ctx->framerate = {fps, 1};
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    ctx->bit_rate = static_cast<int64_t>(bitrate_kbps) * 1000;
    ctx->gop_size = gop_size;

    bool is_vt = name.find("videotoolbox") != std::string::npos;
    if (is_vt) {
        ctx->max_b_frames = 0;
        ctx->color_range = AVCOL_RANGE_MPEG;
    } else {
        ctx->max_b_frames = max_b_frames;
    }

    if (codec->id == AV_CODEC_ID_H264 && name == "libx264") {
        if (crf > 0) {
            av_opt_set_int(ctx->priv_data, "crf", crf, 0);
        }
        av_opt_set(ctx->priv_data, "preset", "fast", 0);
        av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
    }

    int ret = avcodec_open2(ctx, codec, nullptr);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        spdlog::warn("Failed to open encoder '{}' ({}x{}): {}", name, width, height, errbuf);
        avcodec_free_context(&ctx);
        return nullptr;
    }

    return ctx;
}

bool FFmpegEncoder::initialize(const domain::EncoderConfig& config) {
    stored_config_ = config;
    active_encoder_ = "unknown";

    const AVCodec* codec = find_h264_encoder(config.prefer_hardware, active_encoder_);
    if (!codec) {
        spdlog::error("No H264 encoder found");
        return false;
    }

    active_codec_ = codec;
    spdlog::info("FFmpegEncoder selected: {}", active_encoder_);
    return true;
}

static AVCodecContext* try_open_context_with_fallback(const AVCodec* preferred_codec,
                                                       const std::string& preferred_name,
                                                       int width, int height,
                                                       const domain::EncoderConfig& config,
                                                       std::string& out_name) {
    if (preferred_codec) {
        AVCodecContext* ctx = open_codec_context(preferred_codec, width, height, 30,
                                                   config.bitrate_kbps, config.keyframe_interval,
                                                   config.crf, config.max_b_frames, preferred_name);
        if (ctx) {
            out_name = preferred_name;
            return ctx;
        }
    }

    const AVCodec* libx264 = avcodec_find_encoder_by_name("libx264");
    if (libx264 && libx264 != preferred_codec) {
        AVCodecContext* ctx = open_codec_context(libx264, width, height, 30,
                                                   config.bitrate_kbps, config.keyframe_interval,
                                                   config.crf, config.max_b_frames, "libx264");
        if (ctx) {
            out_name = "libx264";
            return ctx;
        }
    }

    const AVCodec* any_h264 = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (any_h264 && any_h264 != preferred_codec && any_h264 != libx264) {
        AVCodecContext* ctx = open_codec_context(any_h264, width, height, 30,
                                                   config.bitrate_kbps, config.keyframe_interval,
                                                   config.crf, config.max_b_frames,
                                                   any_h264->name);
        if (ctx) {
            out_name = any_h264->name;
            return ctx;
        }
    }

    return nullptr;
}

bool FFmpegEncoder::ensure_context(int width, int height) {
    if (enc_ctx_ && width == enc_width_ && height == enc_height_) {
        return true;
    }

    if (enc_ctx_) {
        avcodec_free_context(&enc_ctx_);
    }

    std::string name;
    enc_ctx_ = try_open_context_with_fallback(active_codec_, active_encoder_,
                                                width, height, stored_config_, name);
    if (!enc_ctx_) {
        spdlog::error("Failed to open any encoder context for {}x{}", width, height);
        return false;
    }

    enc_width_ = width;
    enc_height_ = height;
    active_encoder_ = name;

    spdlog::info("FFmpegEncoder context: {} ({}x{})", active_encoder_, width, height);
    return true;
}

std::vector<uint8_t> FFmpegEncoder::encode(const uint8_t* rgb_data, int width, int height, int64_t pts) {
    if (!ensure_context(width, height)) return {};

    if (!sws_ctx_ || sws_src_width_ != width || sws_src_height_ != height) {
        if (sws_ctx_) {
            sws_freeContext(sws_ctx_);
        }
        sws_ctx_ = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                                   enc_width_, enc_height_, AV_PIX_FMT_YUV420P,
                                   SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) {
            spdlog::error("Failed to create swscale context ({}x{} -> {}x{})",
                          width, height, enc_width_, enc_height_);
            return {};
        }
        sws_src_width_ = width;
        sws_src_height_ = height;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) return {};

    frame->format = enc_ctx_->pix_fmt;
    frame->width = enc_width_;
    frame->height = enc_height_;
    frame->pts = pts;

    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        av_frame_free(&frame);
        return {};
    }

    ret = av_frame_make_writable(frame);
    if (ret < 0) {
        av_frame_free(&frame);
        return {};
    }

    const uint8_t* src_data[1] = {rgb_data};
    int src_linesize[1] = {width * 3};

    ret = sws_scale(sws_ctx_, src_data, src_linesize, 0, height,
                    frame->data, frame->linesize);
    if (ret < 0) {
        av_frame_free(&frame);
        return {};
    }

    ret = avcodec_send_frame(enc_ctx_, frame);
    av_frame_free(&frame);

    if (ret < 0) {
        spdlog::warn("avcodec_send_frame failed: {}", ret);
        return {};
    }

    std::vector<uint8_t> result;
    AVPacket* pkt = av_packet_alloc();
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx_, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            spdlog::warn("avcodec_receive_packet failed: {}", ret);
            break;
        }

        result.insert(result.end(), pkt->data, pkt->data + pkt->size);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);

    return result;
}

bool FFmpegEncoder::flush(std::vector<uint8_t>& out) {
    if (!enc_ctx_) return false;

    int ret = avcodec_send_frame(enc_ctx_, nullptr);
    if (ret < 0) return false;

    AVPacket* pkt = av_packet_alloc();
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx_, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) break;

        out.insert(out.end(), pkt->data, pkt->data + pkt->size);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);

    return true;
}

std::string FFmpegEncoder::encoder_name() const {
    return active_encoder_;
}

} // namespace micecam::infrastructure

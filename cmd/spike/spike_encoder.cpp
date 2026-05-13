#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

static const int FRAME_WIDTH = 1280;
static const int FRAME_HEIGHT = 720;
static const int FRAME_FPS = 30;
static const int FRAME_COUNT = 30;

static std::vector<uint8_t> generate_color_bars_yuv420p(int width, int height) {
    std::vector<uint8_t> frame;
    int y_size = width * height;
    int uv_size = width * height / 4;
    frame.resize(y_size + 2 * uv_size);

    uint8_t* y_plane = frame.data();
    uint8_t* u_plane = y_plane + y_size;
    uint8_t* v_plane = u_plane + uv_size;

    const int bar_width = width / 7;
    const uint8_t bar_y[]  = {180, 162, 131, 112, 84, 65, 35};
    const uint8_t bar_u[]  = {128,  44, 156,  72, 184, 100, 212};
    const uint8_t bar_v[]  = {128, 142,  44,  58, 198, 212, 114};

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int bar = (x / bar_width) % 7;
            if (bar >= 7) bar = 6;
            size_t idx = y * width + x;
            y_plane[idx] = bar_y[bar];
            if (y % 2 == 0 && x % 2 == 0) {
                size_t uv_idx = (y / 2) * (width / 2) + (x / 2);
                u_plane[uv_idx] = bar_u[bar];
                v_plane[uv_idx] = bar_v[bar];
            }
        }
    }
    return frame;
}

void run_encoder_spike(bool force_fallback) {
    spdlog::info("=== FFmpeg Encoder Spike ===");

    const AVCodec* codec = nullptr;
    std::string encoder_name;

    if (force_fallback) {
        spdlog::info("Force fallback mode: attempting invalid encoder first...");
        const AVCodec* invalid_codec = avcodec_find_encoder_by_name("h264_nonexistent_encoder");
        if (invalid_codec) {
            spdlog::warn("Unexpectedly found nonexistent encoder");
        } else {
            spdlog::info("Invalid encoder not found (expected), falling back to libx264");
        }
        codec = avcodec_find_encoder_by_name("libx264");
        encoder_name = "libx264";
    } else {
#ifdef __APPLE__
        spdlog::info("macOS detected, trying h264_videotoolbox...");
        codec = avcodec_find_encoder_by_name("h264_videotoolbox");
        if (codec) {
            encoder_name = "h264_videotoolbox";
        } else {
            spdlog::warn("h264_videotoolbox not available");
        }
#else
        spdlog::info("Non-macOS platform, trying platform-specific hardware encoder...");
#endif

        if (!codec) {
            spdlog::info("Trying libx264...");
            codec = avcodec_find_encoder_by_name("libx264");
            if (codec) {
                encoder_name = "libx264";
            }
        }

        if (!codec) {
            spdlog::info("Trying default H264 encoder...");
            codec = avcodec_find_encoder(AV_CODEC_ID_H264);
            if (codec) {
                encoder_name = codec->name;
            }
        }
    }

    if (!codec) {
        spdlog::error("No H264 encoder found on this system");
        spdlog::info("=== FFmpeg Encoder Spike: FAILED ===");
        return;
    }

    spdlog::info("Selected encoder: {}", encoder_name);

    AVCodecContext* enc_ctx = avcodec_alloc_context3(codec);
    if (!enc_ctx) {
        spdlog::error("Failed to allocate encoder context");
        return;
    }

    enc_ctx->width = FRAME_WIDTH;
    enc_ctx->height = FRAME_HEIGHT;
    enc_ctx->time_base = {1, FRAME_FPS};
    enc_ctx->framerate = {FRAME_FPS, 1};
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    enc_ctx->bit_rate = 4000000;
    enc_ctx->gop_size = 30;

    if (encoder_name.find("videotoolbox") != std::string::npos) {
        enc_ctx->max_b_frames = 0;
    } else {
        enc_ctx->max_b_frames = 1;
    }

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(enc_ctx->priv_data, "preset", "fast", 0);
        av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);
    }

    int ret = avcodec_open2(enc_ctx, codec, nullptr);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        spdlog::error("Failed to open encoder: {}", errbuf);

        if (encoder_name == "h264_videotoolbox") {
            spdlog::warn("VideoToolbox failed, falling back to libx264...");
            encoder_name = "libx264";
            codec = avcodec_find_encoder_by_name("libx264");
            if (codec) {
                avcodec_free_context(&enc_ctx);
                enc_ctx = avcodec_alloc_context3(codec);
                enc_ctx->width = FRAME_WIDTH;
                enc_ctx->height = FRAME_HEIGHT;
                enc_ctx->time_base = {1, FRAME_FPS};
                enc_ctx->framerate = {FRAME_FPS, 1};
                enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
                enc_ctx->bit_rate = 4000000;
                enc_ctx->gop_size = 30;
                enc_ctx->max_b_frames = 1;
                av_opt_set(enc_ctx->priv_data, "preset", "fast", 0);
                av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);

                ret = avcodec_open2(enc_ctx, codec, nullptr);
                if (ret < 0) {
                    char errbuf2[256];
                    av_strerror(ret, errbuf2, sizeof(errbuf2));
                    spdlog::error("libx264 fallback also failed: {}", errbuf2);
                    avcodec_free_context(&enc_ctx);
                    spdlog::info("=== FFmpeg Encoder Spike: FAILED ===");
                    return;
                }
                spdlog::info("Fallback to libx264 succeeded");
            } else {
                spdlog::error("libx264 not available for fallback");
                avcodec_free_context(&enc_ctx);
                spdlog::info("=== FFmpeg Encoder Spike: FAILED ===");
                return;
            }
        } else {
            avcodec_free_context(&enc_ctx);
            spdlog::info("=== FFmpeg Encoder Spike: FAILED ===");
            return;
        }
    }

    const char* output_file = "spike_encoder.mp4";
    AVFormatContext* fmt_ctx = nullptr;

    ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, output_file);
    if (ret < 0 || !fmt_ctx) {
        spdlog::error("Failed to allocate output context");
        avcodec_free_context(&enc_ctx);
        return;
    }

    AVStream* stream = avformat_new_stream(fmt_ctx, nullptr);
    if (!stream) {
        spdlog::error("Failed to create stream");
        avformat_free_context(fmt_ctx);
        avcodec_free_context(&enc_ctx);
        return;
    }
    stream->time_base = enc_ctx->time_base;
    stream->id = fmt_ctx->nb_streams - 1;

    ret = avcodec_parameters_from_context(stream->codecpar, enc_ctx);
    if (ret < 0) {
        spdlog::error("Failed to copy codec parameters");
        avformat_free_context(fmt_ctx);
        avcodec_free_context(&enc_ctx);
        return;
    }

    ret = avio_open(&fmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
    if (ret < 0) {
        spdlog::error("Failed to open output file: {}", output_file);
        avformat_free_context(fmt_ctx);
        avcodec_free_context(&enc_ctx);
        return;
    }

    ret = avformat_write_header(fmt_ctx, nullptr);
    if (ret < 0) {
        spdlog::error("Failed to write header");
        avio_closep(&fmt_ctx->pb);
        avformat_free_context(fmt_ctx);
        avcodec_free_context(&enc_ctx);
        return;
    }

    int frame_size = av_image_get_buffer_size(enc_ctx->pix_fmt, enc_ctx->width, enc_ctx->height, 1);

    spdlog::info("Encoding {} frames ({}x{}, {})...", FRAME_COUNT, FRAME_WIDTH, FRAME_HEIGHT, encoder_name);

    int frame_idx = 0;
    for (int i = 0; i < FRAME_COUNT; i++) {
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            spdlog::error("Failed to allocate frame");
            break;
        }

        frame->format = enc_ctx->pix_fmt;
        frame->width = enc_ctx->width;
        frame->height = enc_ctx->height;
        frame->pts = i;

        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            spdlog::error("Failed to allocate frame buffer");
            av_frame_free(&frame);
            break;
        }

        ret = av_frame_make_writable(frame);
        if (ret < 0) {
            spdlog::error("Frame not writable");
            av_frame_free(&frame);
            break;
        }

        auto color_bars = generate_color_bars_yuv420p(FRAME_WIDTH, FRAME_HEIGHT);
        for (int y = 0; y < FRAME_HEIGHT; y++) {
            std::memcpy(
                frame->data[0] + y * frame->linesize[0],
                color_bars.data() + y * FRAME_WIDTH,
                FRAME_WIDTH
            );
        }
        int uv_height = FRAME_HEIGHT / 2;
        int uv_width = FRAME_WIDTH / 2;
        for (int y = 0; y < uv_height; y++) {
            std::memcpy(
                frame->data[1] + y * frame->linesize[1],
                color_bars.data() + FRAME_WIDTH * FRAME_HEIGHT + y * uv_width,
                uv_width
            );
            std::memcpy(
                frame->data[2] + y * frame->linesize[2],
                color_bars.data() + FRAME_WIDTH * FRAME_HEIGHT + uv_width * uv_height + y * uv_width,
                uv_width
            );
        }

        ret = avcodec_send_frame(enc_ctx, frame);
        av_frame_free(&frame);

        if (ret < 0) {
            spdlog::error("Error sending frame {}: {}", i, ret);
            continue;
        }

        AVPacket* pkt = av_packet_alloc();
        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                spdlog::error("Error receiving packet: {}", ret);
                break;
            }

            pkt->stream_index = stream->index;
            av_packet_rescale_ts(pkt, enc_ctx->time_base, stream->time_base);

            ret = av_interleaved_write_frame(fmt_ctx, pkt);
            if (ret < 0) {
                spdlog::error("Error writing frame: {}", ret);
            }

            frame_idx++;
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }

    // Flush encoder
    ret = avcodec_send_frame(enc_ctx, nullptr);
    if (ret >= 0) {
        AVPacket* pkt = av_packet_alloc();
        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) break;

            pkt->stream_index = stream->index;
            av_packet_rescale_ts(pkt, enc_ctx->time_base, stream->time_base);
            av_interleaved_write_frame(fmt_ctx, pkt);
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }

    av_write_trailer(fmt_ctx);
    avio_closep(&fmt_ctx->pb);
    avformat_free_context(fmt_ctx);
    avcodec_free_context(&enc_ctx);

    spdlog::info("=== FFmpeg Encoder Spike: COMPLETE ===");
    spdlog::info("Encoder used: {}", encoder_name);
    spdlog::info("Total encoded frames: {}", frame_idx);
    spdlog::info("Output: {}", output_file);
    spdlog::info("Verify: ffprobe spike_encoder.mp4");
}

#include "FFmpegCameraBackend.h"

#include <algorithm>

#include "domain/Capabilities.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

namespace micecam::infrastructure {

class FFmpegCameraStream : public domain::CameraStream {
public:
    FFmpegCameraStream(AVFormatContext* ctx, int w, int h, int fps, int stream_idx)
        : fmt_ctx_(ctx), width_(w), height_(h), fps_(fps) {
        // Initialize MJPEG decoder
        const AVCodec* dec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
        if (dec) {
            dec_ctx_ = avcodec_alloc_context3(dec);
            dec_ctx_->width = w;
            dec_ctx_->height = h;
            dec_ctx_->pix_fmt = AV_PIX_FMT_YUVJ422P;
            avcodec_open2(dec_ctx_, dec, nullptr);
        }
        // Allocate decoded frame
        dec_frame_ = av_frame_alloc();
    }

    ~FFmpegCameraStream() override { close(); }

    bool read_frame(std::vector<uint8_t>& out_data, int64_t& out_pts) override {
        if (!fmt_ctx_ || !dec_ctx_ || !dec_frame_) return false;

        AVPacket* pkt = av_packet_alloc();
        int ret = av_read_frame(fmt_ctx_, pkt);
        if (ret < 0) {
            av_packet_free(&pkt);
            return false;
        }

        // Decode MJPEG → YUV
        ret = avcodec_send_packet(dec_ctx_, pkt);
        if (ret < 0) {
            av_packet_free(&pkt);
            return false;
        }
        ret = avcodec_receive_frame(dec_ctx_, dec_frame_);
        if (ret < 0) {
            av_packet_free(&pkt);
            return false;
        }

        // Pack YUV planar data into contiguous buffer
        int w = dec_ctx_->width;
        int h = dec_ctx_->height;
        int y_size = w * h;
        int u_size = w * h / 4;
        int v_size = w * h / 4;
        out_data.resize(y_size + u_size + v_size);
        auto* dst = out_data.data();
        for (int i = 0; i < dec_ctx_->height; i++) {
            memcpy(dst, dec_frame_->data[0] + i * dec_frame_->linesize[0], dec_ctx_->width);
            dst += dec_ctx_->width;
        }
        for (int i = 0; i < dec_ctx_->height / 2; i++) {
            memcpy(dst, dec_frame_->data[1] + i * dec_frame_->linesize[1], dec_ctx_->width / 2);
            dst += dec_ctx_->width / 2;
        }
        for (int i = 0; i < dec_ctx_->height / 2; i++) {
            memcpy(dst, dec_frame_->data[2] + i * dec_frame_->linesize[2], dec_ctx_->width / 2);
            dst += dec_ctx_->width / 2;
        }

        out_pts = pkt->pts;
        av_packet_free(&pkt);
        return true;
    }

    int width() const override { return width_; }
    int height() const override { return height_; }
    int fps() const override { return fps_; }
    std::string pixel_format() const override { return "yuv420p"; }
    bool is_open() const override { return fmt_ctx_ != nullptr; }

    void close() override {
        if (dec_frame_) { av_frame_free(&dec_frame_); dec_frame_ = nullptr; }
        if (dec_ctx_) { avcodec_free_context(&dec_ctx_); dec_ctx_ = nullptr; }
        if (fmt_ctx_) {
            avformat_close_input(&fmt_ctx_);
        }
    }

private:
    AVFormatContext* fmt_ctx_;
    AVCodecContext* dec_ctx_ = nullptr;
    AVFrame* dec_frame_ = nullptr;
    int width_, height_, fps_;
};

FFmpegCameraBackend::FFmpegCameraBackend() {
    avdevice_register_all();
}

std::vector<domain::DeviceInfo> FFmpegCameraBackend::enumerate_devices() {
    std::vector<domain::DeviceInfo> result;
#if defined(__APPLE__)
    const char* input_format = "avfoundation";
#elif defined(_WIN32)
    const char* input_format = "dshow";
#else
    const char* input_format = "v4l2";
#endif

    AVDeviceInfoList* dev_list = nullptr;
    const AVInputFormat* fmt = av_find_input_format(input_format);
    if (!fmt) return result;

    AVFormatContext* ctx = avformat_alloc_context();
    if (!ctx) return result;

    int ret = avdevice_list_input_sources(fmt, nullptr, nullptr, &dev_list);
    if (ret >= 0 && dev_list) {
        for (int i = 0; i < dev_list->nb_devices; i++) {
            domain::DeviceInfo info;
            info.id = std::string(input_format) + ":" + std::to_string(i);
            info.name = dev_list->devices[i]->device_name
                ? dev_list->devices[i]->device_name : "Unknown";
            info.type = "usb";
            domain::StreamInfo si;
            si.index = 0;
            si.max_width = 1920;
            si.max_height = 1080;
            si.label = dev_list->devices[i]->device_name
                ? dev_list->devices[i]->device_name : "Unknown";
            si.resolutions = {
                {1920, 1080, "1080p"},
                {1280, 720, "720p"},
            };
            si.supported_formats = {"yuv420p", "mjpeg"};
            si.supported_framerates = {15, 30, 60};
            si.available = true;
            info.streams.push_back(si);
            result.push_back(info);
        }
        avdevice_free_list_devices(&dev_list);
    }
    avformat_free_context(ctx);
    return result;
}

std::unique_ptr<domain::CameraStream> FFmpegCameraBackend::open_stream(const domain::StreamConfig& config) {
    AVFormatContext* ctx = nullptr;
#if defined(__APPLE__)
    const char* input_format = "avfoundation";
    std::string url = std::to_string(config.stream_index);
#elif defined(_WIN32)
    const char* input_format = "dshow";
    std::string url = "video=" + config.device_id;
#else
    const char* input_format = "v4l2";
    std::string url = "/dev/video" + std::to_string(config.stream_index);
#endif

    const AVInputFormat* fmt = av_find_input_format(input_format);
    if (!fmt) return nullptr;

    AVDictionary* opts = nullptr;
    int w = config.width > 0 ? config.width : 1920;
    int h = config.height > 0 ? config.height : 1080;
    int fps = config.framerate > 0 ? config.framerate : 30;

    char size_str[32];
    snprintf(size_str, sizeof(size_str), "%dx%d", w, h);
    av_dict_set(&opts, "video_size", size_str, 0);

    char fps_str[16];
    snprintf(fps_str, sizeof(fps_str), "%d", fps);
    av_dict_set(&opts, "framerate", fps_str, 0);

    if (config.pixel_format == "mjpeg") {
        av_dict_set(&opts, "input_format", "mjpeg", 0);
    }

    int ret = avformat_open_input(&ctx, url.c_str(), fmt, &opts);
    av_dict_free(&opts);

    if (ret < 0) return nullptr;

    int stream_idx = av_find_best_stream(ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (stream_idx < 0) {
        avformat_close_input(&ctx);
        return nullptr;
    }

    return std::make_unique<FFmpegCameraStream>(ctx, w, h, fps, stream_idx);
}

domain::Capabilities FFmpegCameraBackend::get_capabilities() {
    domain::Capabilities caps;
    caps.supports_hardware_encode = false;
    caps.encoder_name = "libx264";
    caps.fallback_encoder_name = "libx264";
    domain::StreamInfo si;
    si.index = 0;
    si.max_width = 4096;
    si.max_height = 2160;
    si.label = "USB Capabilities";
    si.resolutions = {
        {4096, 2160, "4K"},
        {1920, 1080, "1080p"},
    };
    si.supported_formats = {"yuv420p", "mjpeg", "rgb24"};
    si.supported_framerates = {15, 30, 60};
    si.available = true;
    caps.streams.push_back(si);
    return caps;
}

} // namespace micecam::infrastructure

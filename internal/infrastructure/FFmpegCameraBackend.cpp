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
    FFmpegCameraStream(AVFormatContext* ctx, int w, int h, int fps)
        : fmt_ctx_(ctx), width_(w), height_(h), fps_(fps) {}

    ~FFmpegCameraStream() override { close(); }

    bool read_frame(std::vector<uint8_t>& out_data, int64_t& out_pts) override {
        if (!fmt_ctx_) return false;
        AVPacket* pkt = av_packet_alloc();
        int ret = av_read_frame(fmt_ctx_, pkt);
        if (ret < 0) {
            av_packet_free(&pkt);
            return false;
        }
        out_data.assign(pkt->data, pkt->data + pkt->size);
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
        if (fmt_ctx_) {
            avformat_close_input(&fmt_ctx_);
        }
    }

private:
    AVFormatContext* fmt_ctx_;
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
            si.supported_formats = {"yuv420p", "mjpeg"};
            si.supported_framerates = {15, 30, 60};
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

    return std::make_unique<FFmpegCameraStream>(ctx, w, h, fps);
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
    si.supported_formats = {"yuv420p", "mjpeg", "rgb24"};
    si.supported_framerates = {15, 30, 60};
    caps.streams.push_back(si);
    return caps;
}

} // namespace micecam::infrastructure

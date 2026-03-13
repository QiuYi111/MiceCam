#include "micecam/camera/ffmpeg_camera_backend.h"
#include "infrastructure/ffmpeg_device_selector.h"

#include <iostream>
#include <sstream>
#include <vector>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace micecam {

<<<<<<< HEAD
namespace {

#if defined(_WIN32)
std::vector<std::string> enumerate_windows_dshow_video_devices() {
    std::vector<std::string> devices;

    const AVInputFormat* input_format = av_find_input_format("dshow");
    if (!input_format) {
=======
#if defined(_WIN32)
namespace {

std::vector<std::string> list_dshow_video_devices() {
    std::vector<std::string> devices;

    const AVInputFormat* ifmt = av_find_input_format("dshow");
    if (!ifmt) {
>>>>>>> feat/windows-packaging
        return devices;
    }

    AVDeviceInfoList* device_list = nullptr;
<<<<<<< HEAD
    if (avdevice_list_input_sources(input_format, nullptr, nullptr, &device_list) < 0 || !device_list) {
        return devices;
    }

    for (int index = 0; index < device_list->nb_devices; ++index) {
        const AVDeviceInfo* device = device_list->devices[index];
        if (!device || !device->device_name) {
            continue;
        }
        devices.emplace_back(device->device_name);
=======
    if (avdevice_list_input_sources(ifmt, nullptr, nullptr, &device_list) < 0 || !device_list) {
        return devices;
    }

    for (int i = 0; i < device_list->nb_devices; ++i) {
        const auto* dev = device_list->devices[i];
        if (!dev) {
            continue;
        }

        bool has_video = dev->nb_media_types == 0;
        for (int j = 0; j < dev->nb_media_types; ++j) {
            if (dev->media_types[j] == AVMEDIA_TYPE_VIDEO) {
                has_video = true;
                break;
            }
        }

        if (!has_video) {
            continue;
        }

        if (dev->device_name && *dev->device_name) {
            devices.emplace_back(dev->device_name);
        } else if (dev->device_description && *dev->device_description) {
            devices.emplace_back(dev->device_description);
        }
>>>>>>> feat/windows-packaging
    }

    avdevice_free_list_devices(&device_list);
    return devices;
}
<<<<<<< HEAD
#endif

}  // namespace
=======

std::string pick_dshow_video_device(int requested_index) {
    const auto devices = list_dshow_video_devices();
    if (devices.empty()) {
        return {};
    }

    int index = requested_index;
    if (index < 0 || index >= static_cast<int>(devices.size())) {
        index = 0;
    }

    return "video=" + devices[static_cast<size_t>(index)];
}

}  // namespace
#endif
>>>>>>> feat/windows-packaging

FFmpegCameraBackend::FFmpegCameraBackend() {
    avdevice_register_all();
}

FFmpegCameraBackend::~FFmpegCameraBackend() {
    stop();
    close_device();
}

bool FFmpegCameraBackend::initialize(const CameraConfig& config) {
    config_ = config;
    return open_device();
}

bool FFmpegCameraBackend::open_device() {
    close_device();

#if defined(_WIN32)
    const char* format_name = "dshow";
<<<<<<< HEAD
    const auto available_devices = enumerate_windows_dshow_video_devices();
    const auto selected_device_name = resolve_ffmpeg_input_device_name(available_devices, config_.device_id, "video=");
    if (!selected_device_name.has_value()) {
        std::cerr << "FFmpeg: Invalid Windows webcam index " << config_.device_id
                  << " (found " << available_devices.size() << " video device(s))" << std::endl;
        return false;
    }
    std::string device_name = *selected_device_name;
=======
    std::string device_name = pick_dshow_video_device(config_.device_id);
    if (device_name.empty()) {
        std::cerr << "FFmpeg: No Windows dshow video devices found" << std::endl;
        return false;
    }
>>>>>>> feat/windows-packaging
#elif defined(__APPLE__)
    const char* format_name = "avfoundation";
    std::string device_name = std::to_string(config_.device_id) + ":none";
#else
    const char* format_name = "v4l2";
    std::string device_name = "/dev/video" + std::to_string(config_.device_id);
#endif

    const AVInputFormat* ifmt = av_find_input_format(format_name);
    if (!ifmt) {
        std::cerr << "FFmpeg: Failed to find " << format_name << " input format" << std::endl;
        return false;
    }

#if defined(_WIN32)
    std::cerr << "FFmpeg: Opening dshow device " << device_name << std::endl;
#endif

    AVDictionary* options = nullptr;
    // Set resolution
    std::string res = std::to_string(config_.width) + "x" + std::to_string(config_.height);
    av_dict_set(&options, "video_size", res.c_str(), 0);
    // Set FPS
    av_dict_set(&options, "framerate", std::to_string(static_cast<int>(config_.fps)).c_str(), 0);
    // Set buffer size to avoid drops
    av_dict_set(&options, "rtbufsize", "1024M", 0);

#if defined(__APPLE__)
    // Built-in macOS cameras typically expose raw YUV formats via AVFoundation.
    av_dict_set(&options, "pixel_format", "uyvy422", 0);
    av_dict_set(&options, "probesize", "32", 0);
    av_dict_set(&options, "analyzeduration", "0", 0);
    av_dict_set(&options, "fflags", "nobuffer", 0);
#else
    // USB webcams on other platforms often deliver MJPEG directly and should prefer it.
    av_dict_set(&options, "vcodec", "mjpeg", 0);
#endif

    int ret = avformat_open_input(&fmt_ctx_, device_name.c_str(), ifmt, &options);
    av_dict_free(&options);

    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "FFmpeg: Failed to open input device: " << errbuf << std::endl;
        return false;
    }

    if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
        std::cerr << "FFmpeg: Failed to find stream information" << std::endl;
        return false;
    }

    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; i++) {
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            break;
        }
    }

    if (video_stream_index_ == -1) {
        std::cerr << "FFmpeg: Failed to find video stream" << std::endl;
        return false;
    }

    pkt_ = av_packet_alloc();
    return true;
}

void FFmpegCameraBackend::close_device() {
    if (pkt_) {
        av_packet_free(&pkt_);
        pkt_ = nullptr;
    }
    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
        fmt_ctx_ = nullptr;
    }
    video_stream_index_ = -1;
}

bool FFmpegCameraBackend::start() {
    if (!fmt_ctx_) {
        if (!open_device()) return false;
    }
    running_.store(true);
    return true;
}

void FFmpegCameraBackend::stop() {
    running_.store(false);
}

std::unique_ptr<Frame> FFmpegCameraBackend::get_frame() {
    if (!running_.load() || !fmt_ctx_ || !pkt_) return nullptr;

    std::lock_guard<std::mutex> lock(capture_mutex_);

    while (running_.load()) {
        int ret = av_read_frame(fmt_ctx_, pkt_);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) continue;
            break;
        }

        if (pkt_->stream_index == video_stream_index_) {
            auto frame_data = std::make_unique<std::vector<uint8_t>>(pkt_->size);
            std::memcpy(frame_data->data(), pkt_->data, pkt_->size);

            auto frame = std::make_unique<Frame>(frame_count_.fetch_add(1), std::move(frame_data));

            auto codec_id = fmt_ctx_->streams[video_stream_index_]->codecpar->codec_id;
            auto pix_fmt = static_cast<AVPixelFormat>(fmt_ctx_->streams[video_stream_index_]->codecpar->format);

            if (codec_id == AV_CODEC_ID_MJPEG) {
                frame->format = PixelFormat::MJPEG;
            } else if (pix_fmt == AV_PIX_FMT_UYVY422) {
                frame->format = PixelFormat::UYVY422;
            } else if (pix_fmt == AV_PIX_FMT_GRAY8) {
                frame->format = PixelFormat::MONO8;
            }

            frame->width = config_.width;
            frame->height = config_.height;

            av_packet_unref(pkt_);
            return frame;
        }
        av_packet_unref(pkt_);
    }

    return nullptr;
}

PixelFormat FFmpegCameraBackend::get_current_format() const {
    if (!fmt_ctx_ || video_stream_index_ == -1) return PixelFormat::MJPEG;

    auto codec_id = fmt_ctx_->streams[video_stream_index_]->codecpar->codec_id;
    auto pix_fmt = static_cast<AVPixelFormat>(fmt_ctx_->streams[video_stream_index_]->codecpar->format);

    if (codec_id == AV_CODEC_ID_MJPEG) return PixelFormat::MJPEG;
    if (pix_fmt == AV_PIX_FMT_UYVY422) return PixelFormat::UYVY422;
    if (pix_fmt == AV_PIX_FMT_GRAY8)   return PixelFormat::MONO8;

    return PixelFormat::MJPEG;
}

} // namespace micecam

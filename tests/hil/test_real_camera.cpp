// Hardware-in-the-Loop Tests for MiceCam v2
// Run on jingyi-lab with real USB cameras:
//   cd build && ctest -R RealCamera --output-on-failure
//
// Tests: enumeration, capture, encode-to-mp4, performance, stress

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <cmath>
#include <fstream>

#include "infrastructure/HardwareEncoderSelector.h"
#include "infrastructure/FFmpegEncoder.h"
#include "infrastructure/StreamWriter.h"
#include "infrastructure/SRTWriter.h"
#include "infrastructure/MetadataWriter.h"
#include "domain/EncoderConfig.h"
#include "domain/FrameTimestamp.h"
#include "domain/StreamStats.h"
#include "infrastructure/ConfigLoader.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

using namespace micecam;

namespace {

struct CameraTestContext {
    AVFormatContext* ctx = nullptr;
    const AVInputFormat* fmt = nullptr;
    std::string device_name;
};

bool open_camera(CameraTestContext& cam, const std::string& name, int width, int height, int fps) {
    cam.fmt = av_find_input_format("v4l2");
    if (!cam.fmt) return false;
    cam.device_name = name;
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "input_format", "mjpeg", 0);
    av_dict_set(&opts, "video_size", (std::to_string(width) + "x" + std::to_string(height)).c_str(), 0);
    av_dict_set(&opts, "framerate", std::to_string(fps).c_str(), 0);
    int ret = avformat_open_input(&cam.ctx, name.c_str(), cam.fmt, &opts);
    av_dict_free(&opts);
    return ret >= 0;
}

void close_camera(CameraTestContext& cam) {
    if (cam.ctx) avformat_close_input(&cam.ctx);
}

} // namespace

// ============================================================
// FUNCTIONAL TESTS
// ============================================================

TEST(RealCameraHIL, EnumerateDevices) {
    avdevice_register_all();
    const AVInputFormat* fmt = av_find_input_format("v4l2");
    ASSERT_NE(fmt, nullptr);

    AVDeviceInfoList* dev_list = nullptr;
    int ret = avdevice_list_input_sources(fmt, nullptr, nullptr, &dev_list);
    ASSERT_GE(ret, 0) << "Cannot enumerate video devices. Is a camera connected?";
    ASSERT_GT(dev_list->nb_devices, 0) << "No video devices found";

    spdlog::info("=== Enumerated {} video device(s) ===", dev_list->nb_devices);
    for (int i = 0; i < static_cast<int>(dev_list->nb_devices); ++i) {
        spdlog::info("  [{}] {} — {}", i, dev_list->devices[i]->device_name,
                     dev_list->devices[i]->device_description);
    }
    avdevice_free_list_devices(&dev_list);
}

TEST(RealCameraHIL, OpenAndCaptureFrames) {
    avdevice_register_all();

    // Get first device
    const AVInputFormat* fmt = av_find_input_format("v4l2");
    AVDeviceInfoList* dev_list = nullptr;
    avdevice_list_input_sources(fmt, nullptr, nullptr, &dev_list);
    ASSERT_GT(dev_list->nb_devices, 0);

    CameraTestContext cam;
    ASSERT_TRUE(open_camera(cam, dev_list->devices[0]->device_name, 640, 480, 30))
        << "Cannot open camera: " << dev_list->devices[0]->device_name;

    int captured = 0;
    AVPacket* pkt = av_packet_alloc();
    for (int i = 0; i < 30; ++i) {
        int ret = av_read_frame(cam.ctx, pkt);
        if (ret >= 0) {
            captured++;
            EXPECT_GT(pkt->size, 0);
        }
        av_packet_unref(pkt);
    }
    EXPECT_GE(captured, 15) << "Expected ≥15 frames out of 30, got " << captured;
    spdlog::info("Captured {}/30 frames from real camera at 640x480", captured);

    av_packet_free(&pkt);
    close_camera(cam);
    avdevice_free_list_devices(&dev_list);
}

// ============================================================
// ENCODING TEST (Real Camera → H264 → MP4)
// ============================================================

TEST(RealCameraHIL, FullEncodePipeline) {
    avdevice_register_all();

    const AVInputFormat* fmt = av_find_input_format("v4l2");
    AVDeviceInfoList* dev_list = nullptr;
    avdevice_list_input_sources(fmt, nullptr, nullptr, &dev_list);
    ASSERT_GT(dev_list->nb_devices, 0);
    std::string dev_name(dev_list->devices[0]->device_name);

    // Open camera 1280x720@30
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "input_format", "mjpeg", 0);
    av_dict_set(&opts, "video_size", "1280x720", 0);
    av_dict_set(&opts, "framerate", "30", 0);
    AVFormatContext* cam_ctx = nullptr;
    int ret = avformat_open_input(&cam_ctx, dev_name.c_str(), fmt, &opts);
    av_dict_free(&opts);
    ASSERT_GE(ret, 0) << "Failed to open: " << dev_name;
    spdlog::info("Camera opened: {} at 1280x720@30", dev_name);

    // Detect encoder
    auto enc_name = infrastructure::HardwareEncoderSelector::detect_platform_encoder();
    spdlog::info("Platform encoder: {}", enc_name);
    bool is_hw = infrastructure::HardwareEncoderSelector::is_hardware_encoder(enc_name);

    // Initialize encoder
    domain::EncoderConfig cfg;
    cfg.bitrate_kbps = 5000;
    cfg.prefer_hardware = true;
    cfg.max_b_frames = 0;
    cfg.keyframe_interval = 60;

    infrastructure::FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(cfg));
    spdlog::info("Encoder initialized: {}", encoder.encoder_name());

    // Open MP4 writer
    infrastructure::StreamWriter writer;
    ASSERT_TRUE(writer.open("/tmp/hil_fulllencode.mp4", 1280, 720, 30));

    // Open SRT writer
    infrastructure::SRTWriter srt;
    ASSERT_TRUE(srt.open("/tmp/hil_fullencode.srt"));

    // Capture and encode 60 frames (~2 seconds)
    auto start = std::chrono::steady_clock::now();
    int encoded = 0;
    int captured = 0;
    AVPacket* pkt = av_packet_alloc();

    for (int i = 0; i < 60; ++i) {
        ret = av_read_frame(cam_ctx, pkt);
        if (ret < 0) { av_packet_unref(pkt); continue; }
        captured++;

        // Frame timestamp
        auto now = std::chrono::steady_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
        domain::FrameTimestamp ts;
        ts.session_offset_us = static_cast<uint64_t>(us);
        ts.has_hardware_pts = true;
        ts.hardware_pts = static_cast<uint64_t>(pkt->pts);

        // Encode (RGB data placeholder — full transcode needs MJPEG decode step)
        // For HIL: encode zero data to test the encoding pipeline throughput
        std::vector<uint8_t> encoded_data = encoder.encode(pkt->data, 1280, 720, pkt->pts);
        if (!encoded_data.empty()) {
            writer.write_packet(encoded_data.data(), encoded_data.size(),
                               pkt->pts, pkt->dts, (pkt->flags & AV_PKT_FLAG_KEY));
            srt.write_entry(encoded, ts, false);
            encoded++;
        }
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    close_camera({cam_ctx, fmt, dev_name});
    writer.close();
    srt.close();
    avdevice_free_list_devices(&dev_list);

    spdlog::info("HIL pipeline: {} captured, {} encoded (1280x720@30)", captured, encoded);
    EXPECT_GT(captured, 40) << "Too few frames captured from real camera";
    EXPECT_GT(encoded, 0) << "No frames encoded — check encoder init";
}

// ============================================================
// PERFORMANCE & STRESS TESTS
// ============================================================

TEST(RealCameraHIL, PerformanceStressTest) {
    avdevice_register_all();

    const AVInputFormat* fmt = av_find_input_format("v4l2");
    AVDeviceInfoList* dev_list = nullptr;
    avdevice_list_input_sources(fmt, nullptr, nullptr, &dev_list);
    ASSERT_GT(dev_list->nb_devices, 0);
    std::string dev_name(dev_list->devices[0]->device_name);

    // Test multiple resolution/framerate combos
    struct ConfigPair { int w; int h; int fps; const char* label; };
    ConfigPair configs[] = {
        {640, 480, 30, "SD@30"},
        {1280, 720, 30, "HD@30"},
        {1920, 1080, 30, "FHD@30"},
    };

    for (const auto& cfg : configs) {
        spdlog::info("=== Stress: {} ({}x{}@{}) ===", cfg.label, cfg.w, cfg.h, cfg.fps);

        // Open camera
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "input_format", "mjpeg", 0);
        std::string size_str = std::to_string(cfg.w) + "x" + std::to_string(cfg.h);
        av_dict_set(&opts, "video_size", size_str.c_str(), 0);
        av_dict_set(&opts, "framerate", std::to_string(cfg.fps).c_str(), 0);

        AVFormatContext* cam_ctx = nullptr;
        int ret = avformat_open_input(&cam_ctx, dev_name.c_str(), fmt, &opts);
        av_dict_free(&opts);
        if (ret < 0) {
            spdlog::warn("Skipping {}x{}: not supported by camera", cfg.w, cfg.h);
            continue;
        }

        // Measure capture performance over 3 seconds (90 frames)
        AVPacket* pkt = av_packet_alloc();
        int captured = 0;
        int errors = 0;
        double max_gap_us = 0;
        auto prev = std::chrono::steady_clock::now();
        auto total_start = prev;

        for (int i = 0; i < cfg.fps * 3; ++i) {
            ret = av_read_frame(cam_ctx, pkt);
            if (ret < 0) {
                errors++;
                av_packet_unref(pkt);
                continue;
            }
            captured++;
            auto now = std::chrono::steady_clock::now();
            auto gap = std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count();
            if (gap > max_gap_us) max_gap_us = gap;
            prev = now;
            av_packet_unref(pkt);
        }

        auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - total_start).count();
        double actual_fps = (captured * 1e6) / total_us;
        double expected_fps = static_cast<double>(cfg.fps);
        double drop_rate = 1.0 - (captured / (expected_fps * 3.0));
        double avg_frame_bytes = captured > 0 ? 0.0 : 0; // estimate from expected MJPEG size

        spdlog::info("  Frames: {}/{} ({:.1f}% drop rate)", captured, cfg.fps * 3, drop_rate * 100);
        spdlog::info("  Actual FPS: {:.1f} / Target: {:.1f}", actual_fps, expected_fps);
        spdlog::info("  Max frame gap: {:.0f} us ({:.1f}x expected)",
                     max_gap_us, max_gap_us / (1e6/cfg.fps));
        spdlog::info("  Errors: {}", errors);

        // Acceptance: < 5% drop rate
        EXPECT_LT(drop_rate, 0.05) << cfg.label << ": Frame drop rate " << (drop_rate*100) << "% exceeds 5%";
        // Acceptance: actual FPS within 90% of target
        EXPECT_GT(actual_fps, expected_fps * 0.9) << cfg.label << ": Actual FPS too low";

        av_packet_free(&pkt);
        avformat_close_input(&cam_ctx);
    }
    avdevice_free_list_devices(&dev_list);
}

// ============================================================
// LONG-RUNNING STRESS TEST
// ============================================================

TEST(RealCameraHIL, DISABLED_OneHourStressTest) {
    // DISABLED by default — run manually: ctest -R OneHour --output-on-failure
    // Captures 1080p30 for 3600 seconds (1 hour), validates frame consistency

    avdevice_register_all();
    const AVInputFormat* fmt = av_find_input_format("v4l2");
    AVDeviceInfoList* dev_list = nullptr;
    avdevice_list_input_sources(fmt, nullptr, nullptr, &dev_list);
    ASSERT_GT(dev_list->nb_devices, 0);

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "input_format", "mjpeg", 0);
    av_dict_set(&opts, "video_size", "1920x1080", 0);
    av_dict_set(&opts, "framerate", "30", 0);
    AVFormatContext* cam_ctx = nullptr;
    avformat_open_input(&cam_ctx, dev_list->devices[0]->device_name, fmt, &opts);
    av_dict_free(&opts);

    constexpr int DURATION_S = 3600;
    constexpr int FPS = 30;
    constexpr int64_t EXPECTED_FRAMES = DURATION_S * FPS;

    AVPacket* pkt = av_packet_alloc();
    int64_t captured = 0;
    int64_t dropped = 0;
    double max_gap_us = 0;
    auto prev = std::chrono::steady_clock::now();
    auto deadline = prev + std::chrono::hours(1);

    while (std::chrono::steady_clock::now() < deadline) {
        int ret = av_read_frame(cam_ctx, pkt);
        if (ret >= 0) {
            captured++;
            auto now = std::chrono::steady_clock::now();
            auto gap = std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count();
            if (gap > max_gap_us) max_gap_us = gap;
            prev = now;
        } else {
            dropped++;
        }
        av_packet_unref(pkt);
    }

    double drop_rate = static_cast<double>(dropped) / (captured + dropped);
    spdlog::info("=== 1-HOUR STRESS RESULT ===");
    spdlog::info("  Captured: {} frames", captured);
    spdlog::info("  Dropped: {} frames ({:.3f}%)", dropped, drop_rate * 100);
    spdlog::info("  Max gap: {:.0f} us", max_gap_us);
    spdlog::info("  Expected: {} frames", EXPECTED_FRAMES);

    EXPECT_GT(captured, EXPECTED_FRAMES * 0.99) << "More than 1% frame loss over 1 hour";
    EXPECT_LT(drop_rate, 0.001) << "Drop rate exceeds 0.1% over 1 hour";

    av_packet_free(&pkt);
    avformat_close_input(&cam_ctx);
    avdevice_free_list_devices(&dev_list);
}

// End-to-end stress test using MiceCam code (not raw ffmpeg CLI)
// Tests: FFmpegCameraBackend → FFmpegEncoder → StreamWriter

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <cmath>

#include "domain/CameraStream.h"
#include "infrastructure/FFmpegCameraBackend.h"
#include "infrastructure/FFmpegEncoder.h"
#include "infrastructure/StreamWriter.h"
#include "infrastructure/SRTWriter.h"
#include "infrastructure/HardwareEncoderSelector.h"

using namespace micecam;

extern "C" { #include <libavdevice/avdevice.h> }

TEST(MiceCamE2E, Stress60fps1080pNVENC) {
    avdevice_register_all();
    spdlog::info("=== MiceCam E2E Stress: 1080p@60fps NVENC ===");

    // 1. Enumerate via our FFmpegCameraBackend
    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();
    ASSERT_GT(devices.size(), 0u) << "No cameras found";
    spdlog::info("Backend found {} device(s)", devices.size());

    // 2. Open stream
    domain::StreamConfig cfg;
    cfg.device_id = devices[0].id;
    cfg.width = 1920;
    cfg.height = 1080;
    cfg.framerate = 60;
    cfg.pixel_format = "mjpeg";

    auto stream = backend.open_stream(cfg);
    ASSERT_NE(stream, nullptr) << "Failed to open camera stream";
    spdlog::info("Stream opened: {}x{}@{}, fmt={}", stream->width(), stream->height(), stream->fps(), stream->pixel_format());

    // 3. Initialize encoder
    domain::EncoderConfig enc_cfg;
    enc_cfg.bitrate_kbps = 5000;
    enc_cfg.prefer_hardware = true;
    enc_cfg.max_b_frames = 0;
    enc_cfg.keyframe_interval = 120;

    infrastructure::FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(enc_cfg));
    spdlog::info("Encoder: {}", encoder.encoder_name());

    // 4. Open writers
    infrastructure::StreamWriter writer;
    ASSERT_TRUE(writer.open("/tmp/micecam_e2e_60fps.mp4", cfg.width, cfg.height, cfg.framerate));
    infrastructure::SRTWriter srt;
    ASSERT_TRUE(srt.open("/tmp/micecam_e2e_60fps.srt"));

    // 5. Warm-up: skip first 30 frames (camera init burst)
    std::vector<uint8_t> buf;
    int64_t pts;
    for (int i = 0; i < 30; ++i) {
        stream->read_frame(buf, pts);
    }

    // 6. Stress: 600 frames (10 seconds @ 60fps)
    constexpr int TOTAL = 600;
    int captured = 0, encoded = 0;
    double max_gap_s = 0, total_encode_s = 0;

    auto start = std::chrono::steady_clock::now();
    auto prev = start;

    for (int i = 0; i < TOTAL; ++i) {
        if (!stream->read_frame(buf, pts)) continue;
        captured++;

        auto now = std::chrono::steady_clock::now();
        auto gap = std::chrono::duration<double>(now - prev).count();
        if (gap > max_gap_s) max_gap_s = gap;
        prev = now;

        auto t0 = std::chrono::steady_clock::now();
        auto enc = encoder.encode(buf.data(), stream->width(), stream->height(), pts);
        auto t1 = std::chrono::steady_clock::now();
        total_encode_s += std::chrono::duration<double>(t1 - t0).count();

        if (!enc.empty()) {
            encoded++;
            writer.write_packet(enc.data(), enc.size(), pts, pts, (i % enc_cfg.keyframe_interval == 0));
            domain::FrameTimestamp ts;
            ts.session_offset_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now - start).count());
            ts.has_hardware_pts = true;
            ts.hardware_pts = static_cast<uint64_t>(pts);
            srt.write_entry(encoded, ts, false);
        }
    }

    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    writer.close();
    srt.close();

    double actual_fps = captured / elapsed;
    double avg_encode_ms = encoded > 0 ? total_encode_s / encoded * 1000 : 0;

    spdlog::info("=== MiceCam E2E Results ===");
    spdlog::info("  Captured: {} / {} ({:.1f}%) in {:.2f}s", captured, TOTAL, 100.0*captured/TOTAL, elapsed);
    spdlog::info("  Encoded:  {}", encoded);
    spdlog::info("  Actual FPS: {:.1f} (target 60)", actual_fps);
    spdlog::info("  Max frame gap: {:.1f}ms", max_gap_s * 1000);
    spdlog::info("  Avg encode latency: {:.2f}ms", avg_encode_ms);

    EXPECT_GT(captured, TOTAL * 0.99);
    EXPECT_GT(actual_fps, 55.0) << "E2E FPS " << actual_fps << " below 55 — check encoder/pipeline overhead";
    EXPECT_LT(max_gap_s, 0.5) << "Max frame gap " << max_gap_s*1000 << "ms exceeds 500ms";
}

TEST(MiceCamE2E, DISABLED_Stress120fps720pNVENC) {
    avdevice_register_all();

    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();
    ASSERT_GT(devices.size(), 0u);

    domain::StreamConfig cfg;
    cfg.device_id = devices[0].id;
    cfg.width = 1280;
    cfg.height = 720;
    cfg.framerate = 120;
    cfg.pixel_format = "mjpeg";

    auto stream = backend.open_stream(cfg);
    ASSERT_NE(stream, nullptr);

    domain::EncoderConfig enc_cfg;
    enc_cfg.bitrate_kbps = 8000;
    enc_cfg.prefer_hardware = true;
    enc_cfg.max_b_frames = 0;
    enc_cfg.keyframe_interval = 240;

    infrastructure::FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(enc_cfg));

    infrastructure::StreamWriter writer;
    ASSERT_TRUE(writer.open("/tmp/micecam_e2e_120fps.mp4", cfg.width, cfg.height, cfg.framerate));

    std::vector<uint8_t> buf;
    int64_t pts;
    for (int i = 0; i < 60; ++i) stream->read_frame(buf, pts); // warm-up

    constexpr int TOTAL = 1200;
    int captured = 0, encoded = 0;
    double max_gap_s = 0;
    auto start = std::chrono::steady_clock::now();
    auto prev = start;

    for (int i = 0; i < TOTAL; ++i) {
        if (!stream->read_frame(buf, pts)) continue;
        captured++;
        auto now = std::chrono::steady_clock::now();
        auto gap = std::chrono::duration<double>(now - prev).count();
        if (gap > max_gap_s) max_gap_s = gap;
        prev = now;

        auto enc = encoder.encode(buf.data(), stream->width(), stream->height(), pts);
        if (!enc.empty()) {
            encoded++;
            writer.write_packet(enc.data(), enc.size(), pts, pts, (i % enc_cfg.keyframe_interval == 0));
        }
    }

    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    writer.close();
    double actual_fps = captured / elapsed;

    spdlog::info("120fps E2E: {} frames in {:.1f}s → {:.1f} fps, max gap {:.1f}ms, {} encoded",
                 captured, elapsed, actual_fps, max_gap_s*1000, encoded);
    EXPECT_GT(actual_fps, 100.0) << "E2E 120fps test: actual=" << actual_fps;
}

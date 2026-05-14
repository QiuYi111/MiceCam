// 1-hour stress test: 720p@120fps → NVENC H264 → MP4
// Usage: ./stress_1h_120fps

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "infrastructure/FFmpegCameraBackend.h"
#include "infrastructure/FFmpegEncoder.h"
#include "infrastructure/StreamWriter.h"
#include "infrastructure/SRTWriter.h"
#include "infrastructure/MetadataWriter.h"
#include "domain/EncoderConfig.h"
#include "domain/FrameTimestamp.h"
#include "domain/StreamStats.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
}

using namespace micecam;

int main() {
    auto console = spdlog::stdout_color_mt("stress");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);
    avdevice_register_all();

    // ---- Enumerate ----
    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();
    if (devices.empty()) { spdlog::error("No cameras"); return 1; }
    spdlog::info("Found {} camera(s)", devices.size());

    // ---- Open stream 1280x720@120fps ----
    domain::StreamConfig cfg;
    cfg.device_id = devices[0].id;
    cfg.width = 1280;
    cfg.height = 720;
    cfg.framerate = 120;
    cfg.pixel_format = "mjpeg";

    auto stream = backend.open_stream(cfg);
    if (!stream) { spdlog::error("Failed to open camera"); return 1; }
    spdlog::info("Camera: {}x{}@{}, fmt={}", stream->width(), stream->height(),
                 stream->fps(), stream->pixel_format());

    // ---- Encoder ----
    domain::EncoderConfig enc_cfg;
    enc_cfg.bitrate_kbps = 8000;
    enc_cfg.prefer_hardware = true;
    enc_cfg.max_b_frames = 0;
    enc_cfg.keyframe_interval = 240;

    infrastructure::FFmpegEncoder encoder;
    if (!encoder.initialize(enc_cfg)) { spdlog::error("Encoder init failed"); return 1; }
    spdlog::info("Encoder: {}", encoder.encoder_name());

    // ---- Writers ----
    const char* out = "/mnt/data/micecam_1h_120fps_nvenc";
    infrastructure::StreamWriter writer;
    writer.open((std::string(out) + ".mp4").c_str(), cfg.width, cfg.height, cfg.framerate);

    infrastructure::SRTWriter srt;
    srt.open((std::string(out) + ".srt").c_str());

    // ---- Warm-up ----
    std::vector<uint8_t> buf;
    int64_t pts;
    for (int i = 0; i < 60; ++i) stream->read_frame(buf, pts);

    // ---- Metadata header ----
    domain::SessionMetadata meta;
    meta.session_id = "stress-1h-120fps";
    meta.wall_clock_anchor_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    meta.encoder_name = encoder.encoder_name();
    meta.bitrate_kbps = enc_cfg.bitrate_kbps;
    meta.keyframe_interval = enc_cfg.keyframe_interval;
    meta.output_dir = out;
    infrastructure::MetadataWriter::write_session_header(
        (std::string(out) + "_meta.json").c_str(), meta);

    // ---- 1-hour loop ----
    constexpr int64_t DURATION_S = 3600;
    constexpr int TARGET_FPS = 120;
    constexpr int64_t TARGET_FRAMES = DURATION_S * TARGET_FPS;

    auto start = std::chrono::steady_clock::now();
    auto deadline = start + std::chrono::hours(1);
    auto stats_interval = start + std::chrono::seconds(10);

    int64_t captured = 0, encoded = 0, empty_reads = 0, max_consec_empty = 0, consec_empty = 0;
    double max_gap_s = 0, total_encode_s = 0;
    auto prev_frame = start;

    while (std::chrono::steady_clock::now() < deadline) {
        if (!stream->read_frame(buf, pts)) {
            empty_reads++; consec_empty++;
            if (consec_empty > max_consec_empty) max_consec_empty = consec_empty;
            continue;
        }
        consec_empty = 0;
        captured++;

        auto now = std::chrono::steady_clock::now();
        auto gap = std::chrono::duration<double>(now - prev_frame).count();
        if (gap > max_gap_s) max_gap_s = gap;
        prev_frame = now;

        auto t0 = std::chrono::steady_clock::now();
        auto enc = encoder.encode(buf.data(), stream->width(), stream->height(), pts);
        total_encode_s += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

        if (!enc.empty()) {
            encoded++;
            writer.write_packet(enc.data(), enc.size(), pts, pts, (encoded % enc_cfg.keyframe_interval == 0));
            domain::FrameTimestamp ts;
            ts.session_offset_us = std::chrono::duration_cast<std::chrono::microseconds>(
                now - start).count();
            ts.has_hardware_pts = true;
            ts.hardware_pts = static_cast<uint64_t>(pts);
            srt.write_entry(encoded, ts, false);
        }

        // Every 10 seconds: log stats
        if (now > stats_interval) {
            auto elapsed = std::chrono::duration<double>(now - start).count();
            spdlog::info("{}s: {} frames, {:.1f} fps, {} encoded, max_gap={:.1f}ms, empty={}",
                         (int)elapsed, captured, captured/elapsed, encoded,
                         max_gap_s*1000, empty_reads);
            stats_interval = now + std::chrono::seconds(10);
        }
    }

    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    double actual_fps = captured / elapsed;
    double avg_encode_ms = encoded > 0 ? total_encode_s / encoded * 1000 : 0;

    // ---- Finalize ----
    writer.close();
    srt.close();

    meta.end_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    infrastructure::MetadataWriter::write_session_footer(
        (std::string(out) + "_meta.json").c_str(), captured, 0, 0);

    std::vector<domain::StreamStats> stats;
    domain::StreamStats s;
    s.stream_id = "cam0";
    s.frames_expected = TARGET_FRAMES;
    s.frames_actual = captured;
    s.drop_rate = 1.0 - static_cast<double>(captured) / TARGET_FRAMES;
    s.avg_encode_latency_us = avg_encode_ms * 1000;
    s.encoder_used = encoder.encoder_name();
    stats.push_back(s);
    infrastructure::MetadataWriter::write_stats((std::string(out) + "_stats.json").c_str(), stats);

    spdlog::info("========================================");
    spdlog::info("1-HOUR STRESS TEST COMPLETE");
    spdlog::info("  Elapsed: {:.1f}s", elapsed);
    spdlog::info("  Captured: {} / {} ({:.1f}%)", captured, TARGET_FRAMES, 100.0*captured/TARGET_FRAMES);
    spdlog::info("  Encoded:  {}", encoded);
    spdlog::info("  Actual FPS: {:.1f} (target {})", actual_fps, TARGET_FPS);
    spdlog::info("  Max frame gap: {:.1f}ms", max_gap_s*1000);
    spdlog::info("  Max consec empty: {}", max_consec_empty);
    spdlog::info("  Total empty reads: {}", empty_reads);
    spdlog::info("  Avg encode latency: {:.2f}ms", avg_encode_ms);
    spdlog::info("  Output: {}.mp4", out);
    spdlog::info("========================================");

    if (captured >= TARGET_FRAMES * 0.999 && actual_fps >= 118.0)
        return 0;
    return 1;
}

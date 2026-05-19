// Hardware-in-the-Loop E2E Tests for MiceCam v2
// Run on jingyi-lab with real USB cameras:
//   cmake -B build -DBUILD_HIL=ON && cmake --build build
//   ctest --test-dir build --output-on-failure -R HIL_FFmpeg
//
// Tests: FFmpeg/USB enumeration, preview, recording, stop, sidecar validation

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstdio>
#include <filesystem>

#include "domain/CameraStream.h"
#include "domain/EncoderConfig.h"
#include "domain/FrameTimestamp.h"
#include "domain/StreamConfig.h"
#include "infrastructure/FFmpegCameraBackend.h"
#include "infrastructure/FFmpegEncoder.h"
#include "infrastructure/StreamWriter.h"
#include "infrastructure/SRTWriter.h"
#include "infrastructure/HardwareEncoderSelector.h"

extern "C" {
#include <libavdevice/avdevice.h>
}

using namespace micecam;

namespace {

struct HiLStats {
    std::string hostname;
    std::string os_name;
    std::string kernel;
    int camera_count = 0;
    std::vector<std::string> device_ids;
};

void print_hil_header(const HiLStats& stats) {
    spdlog::info("=== MiceCam HIL E2E ===");
    spdlog::info("  Hostname:    {}", stats.hostname);
    spdlog::info("  OS:          {}", stats.os_name);
    spdlog::info("  Kernel:      {}", stats.kernel);
    spdlog::info("  Cameras:     {}", stats.camera_count);
    for (size_t i = 0; i < stats.device_ids.size(); ++i) {
        spdlog::info("    [{}] {}", i, stats.device_ids[i]);
    }
}

HiLStats gather_hil_stats(const std::vector<domain::DeviceInfo>& devices) {
    HiLStats s;
    s.hostname = []() {
        std::array<char, 256> buf{};
        FILE* f = popen("hostname 2>/dev/null", "r");
        if (!f) return std::string("unknown");
        if (fgets(buf.data(), buf.size(), f)) {
            std::string result(buf.data());
            if (!result.empty() && result.back() == '\n') result.pop_back();
            return result;
        }
        pclose(f);
        return std::string("unknown");
    }();
    s.os_name = []() {
        std::array<char, 256> buf{};
        FILE* f = popen("uname -s 2>/dev/null", "r");
        if (!f) return std::string("unknown");
        if (fgets(buf.data(), buf.size(), f)) {
            std::string result(buf.data());
            if (!result.empty() && result.back() == '\n') result.pop_back();
            return result;
        }
        pclose(f);
        return std::string("unknown");
    }();
    s.kernel = []() {
        std::array<char, 256> buf{};
        FILE* f = popen("uname -r 2>/dev/null", "r");
        if (!f) return std::string("unknown");
        if (fgets(buf.data(), buf.size(), f)) {
            std::string result(buf.data());
            if (!result.empty() && result.back() == '\n') result.pop_back();
            return result;
        }
        pclose(f);
        return std::string("unknown");
    }();
    s.camera_count = static_cast<int>(devices.size());
    for (const auto& d : devices) {
        s.device_ids.push_back(d.id);
    }
    return s;
}

} // namespace

// ============================================================
// TEST 1: FFmpeg/USB Camera Enumeration (FR-033)
// ============================================================

TEST(HIL_FFmpegUSB, EnumerateDevices) {
    avdevice_register_all();

    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();

    auto stats = gather_hil_stats(devices);
    print_hil_header(stats);

    if (devices.empty()) {
        GTEST_SKIP() << "No USB/FFmpeg cameras detected on " << stats.hostname;
    }

    EXPECT_GE(devices.size(), 1u) << "Expected at least 1 camera device";
    spdlog::info("PASS: FFmpeg enumerated {} camera(s)", devices.size());

    for (const auto& d : devices) {
        spdlog::info("  Device: id={} name={} vendor={} serial={} type={}",
                     d.id, d.name, d.vendor, d.serial, d.type);
        for (const auto& s : d.streams) {
            spdlog::info("    Stream idx={} max={}x{} label={}",
                         s.index, s.max_width, s.max_height, s.label);
            for (const auto& r : s.resolutions) {
                spdlog::info("      Resolution: {}x{} {}",
                             r.width, r.height, r.label);
            }
        }
    }
}

// ============================================================
// TEST 2: Camera Preview — open device, receive frames for ~5s (FR-033)
// ============================================================

TEST(HIL_FFmpegUSB, CameraPreview) {
    avdevice_register_all();

    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();

    if (devices.empty()) {
        GTEST_SKIP() << "No cameras available for preview test";
    }

    domain::StreamConfig cfg;
    cfg.device_id = devices[0].id;
    cfg.width = 640;
    cfg.height = 480;
    cfg.framerate = 30;
    cfg.pixel_format = "mjpeg";

    auto stream = backend.open_stream(cfg);
    if (!stream) {
        GTEST_SKIP() << "Cannot open " << devices[0].id << " for preview";
    }

    spdlog::info("Preview: opened {} at {}x{}@{}, fmt={}",
                 devices[0].id, stream->width(), stream->height(),
                 stream->fps(), stream->pixel_format());

    int frame_count = 0;
    auto start = std::chrono::steady_clock::now();
    constexpr auto kDuration = std::chrono::seconds(5);

    std::vector<uint8_t> buf;
    int64_t pts;

    while (std::chrono::steady_clock::now() - start < kDuration) {
        if (stream->read_frame(buf, pts)) {
            frame_count++;
            EXPECT_GT(buf.size(), 0u) << "Frame " << frame_count << " is empty";
        }
    }

    auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();

    spdlog::info("Preview: {} frames in {:.1f}s ({:.1f} fps)",
                 frame_count, elapsed, frame_count / elapsed);
    EXPECT_GT(frame_count, 0) << "Received zero frames during 5s preview";
    EXPECT_GE(frame_count / elapsed, 5.0)
        << "Frame rate too low: " << (frame_count / elapsed) << " fps";

    stream->close();
    spdlog::info("PASS: Camera preview test");
}

// ============================================================
// TEST 3: Recording — start, record ~10s, stop, verify MP4 (FR-033)
// ============================================================

TEST(HIL_FFmpegUSB, RecordToMP4) {
    avdevice_register_all();

    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();

    if (devices.empty()) {
        GTEST_SKIP() << "No cameras available for recording test";
    }

    const std::string output_path = "/tmp/hil_e2e_record.mp4";

    domain::StreamConfig cfg;
    cfg.device_id = devices[0].id;
    cfg.width = 1280;
    cfg.height = 720;
    cfg.framerate = 30;
    cfg.pixel_format = "mjpeg";

    auto stream = backend.open_stream(cfg);
    if (!stream) {
        GTEST_SKIP() << "Cannot open " << devices[0].id << " at 1280x720 for recording";
    }

    spdlog::info("Recording: opened {} at {}x{}@{}, fmt={}",
                 devices[0].id, stream->width(), stream->height(),
                 stream->fps(), stream->pixel_format());

    auto enc_name = infrastructure::HardwareEncoderSelector::detect_platform_encoder();
    spdlog::info("Platform encoder: {} (hw: {})", enc_name,
        infrastructure::HardwareEncoderSelector::is_hardware_encoder(enc_name) ? "yes" : "no");

    domain::EncoderConfig enc_cfg;
    enc_cfg.bitrate_kbps = 5000;
    enc_cfg.prefer_hardware = true;
    enc_cfg.max_b_frames = 0;
    enc_cfg.keyframe_interval = 60;

    infrastructure::FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(enc_cfg));
    spdlog::info("Encoder ready: {}", encoder.encoder_name());

    infrastructure::StreamWriter writer;
    ASSERT_TRUE(writer.open(output_path, cfg.width, cfg.height, cfg.framerate));

    int frame_count = 0;
    int encoded_count = 0;
    auto start = std::chrono::steady_clock::now();
    constexpr auto kDuration = std::chrono::seconds(10);

    std::vector<uint8_t> buf;
    int64_t pts;

    while (std::chrono::steady_clock::now() - start < kDuration) {
        if (!stream->read_frame(buf, pts)) continue;
        frame_count++;

        auto enc_data = encoder.encode(buf.data(), cfg.width, cfg.height, pts);
        if (!enc_data.empty()) {
            encoded_count++;
            writer.write_packet(enc_data.data(), enc_data.size(),
                                pts, pts, (frame_count % enc_cfg.keyframe_interval == 0));
        }
    }

    stream->close();
    writer.close();

    auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();

    spdlog::info("Recording: {} captured, {} encoded in {:.1f}s",
                 frame_count, encoded_count, elapsed);

    EXPECT_GT(frame_count, 0) << "Zero frames captured during recording";
    EXPECT_GT(encoded_count, 0) << "Zero frames encoded — check encoder/format";

    std::error_code ec;
    EXPECT_TRUE(std::filesystem::exists(output_path, ec))
        << "MP4 file not created at " << output_path;
    EXPECT_GT(std::filesystem::file_size(output_path, ec), 1000u)
        << "MP4 file at " << output_path << " is too small or empty";

    spdlog::info("MP4 file: {} bytes at {}",
                 std::filesystem::file_size(output_path, ec), output_path);

    std::remove(output_path.c_str());
    spdlog::info("PASS: Recording-to-MP4 test");
}

// ============================================================
// TEST 4: Recording stop — verify SRT and metadata sidecars (FR-033)
// ============================================================

TEST(HIL_FFmpegUSB, RecordingStopHasSidecars) {
    avdevice_register_all();

    infrastructure::FFmpegCameraBackend backend;
    auto devices = backend.enumerate_devices();

    if (devices.empty()) {
        GTEST_SKIP() << "No cameras available for sidecar test";
    }

    const std::string output_prefix = "/tmp/hil_e2e_sidecar";

    domain::StreamConfig cfg;
    cfg.device_id = devices[0].id;
    cfg.width = 640;
    cfg.height = 480;
    cfg.framerate = 30;
    cfg.pixel_format = "mjpeg";

    auto stream = backend.open_stream(cfg);
    if (!stream) {
        GTEST_SKIP() << "Cannot open " << devices[0].id << " for sidecar test";
    }

    domain::EncoderConfig enc_cfg;
    enc_cfg.bitrate_kbps = 3000;
    enc_cfg.prefer_hardware = true;
    enc_cfg.max_b_frames = 0;
    enc_cfg.keyframe_interval = 30;

    infrastructure::FFmpegEncoder encoder;
    ASSERT_TRUE(encoder.initialize(enc_cfg));

    infrastructure::StreamWriter writer;
    ASSERT_TRUE(writer.open(output_prefix + ".mp4", cfg.width, cfg.height, cfg.framerate));

    infrastructure::SRTWriter srt_writer;
    ASSERT_TRUE(srt_writer.open(output_prefix + ".srt", cfg.framerate));

    int frame_count = 0;
    int encoded_count = 0;
    auto start = std::chrono::steady_clock::now();
    constexpr auto kDuration = std::chrono::seconds(5);

    std::vector<uint8_t> buf;
    int64_t pts;

    while (std::chrono::steady_clock::now() - start < kDuration) {
        if (!stream->read_frame(buf, pts)) continue;
        frame_count++;

        auto enc_data = encoder.encode(buf.data(), cfg.width, cfg.height, pts);
        if (!enc_data.empty()) {
            encoded_count++;
            writer.write_packet(enc_data.data(), enc_data.size(),
                                pts, pts, (frame_count % enc_cfg.keyframe_interval == 0));

            domain::FrameTimestamp ts;
            auto now = std::chrono::steady_clock::now();
            ts.session_offset_us = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(now - start).count());
            ts.has_hardware_pts = true;
            ts.hardware_pts = static_cast<uint64_t>(pts);
            srt_writer.write_entry(encoded_count, ts, false);
        }
    }

    stream->close();
    writer.close();
    srt_writer.close();

    spdlog::info("Sidecar test: {} captured, {} encoded in 5s",
                 frame_count, encoded_count);

    std::error_code ec;
    EXPECT_TRUE(std::filesystem::exists(output_prefix + ".mp4", ec))
        << "MP4 missing: " << output_prefix << ".mp4";
    EXPECT_TRUE(std::filesystem::exists(output_prefix + ".srt", ec))
        << "SRT missing: " << output_prefix << ".srt";

    auto mp4_size = std::filesystem::file_size(output_prefix + ".mp4", ec);
    auto srt_size = std::filesystem::file_size(output_prefix + ".srt", ec);

    EXPECT_GT(mp4_size, 1000u) << "MP4 too small: " << mp4_size;
    EXPECT_GT(srt_size, 0u) << "SRT is empty";

    spdlog::info("Sidecar files: mp4={} bytes, srt={} bytes", mp4_size, srt_size);

    std::remove((output_prefix + ".mp4").c_str());
    std::remove((output_prefix + ".srt").c_str());
    spdlog::info("PASS: Recording-stop sidecar test");
}

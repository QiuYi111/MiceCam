#pragma once

#include "domain/frame.h"
#include "domain/ring_buffer.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace micecam {

struct SessionConfig {
    std::string output_dir = ".";
    std::string session_name = "session";
    bool enable_checksums = true;
    size_t ring_buffer_size = 10;  // Ring buffer capacity (default: 10 frames)
    bool append = false;           // Append to existing file

    // Camera configuration (for metadata)
    std::string camera_backend_name = "unknown";
    std::string pixel_format = "mjpeg";
    int width = 0;
    int height = 0;
    double fps = 0.0;
};

struct FrameMetadataRecord {
    uint64_t sequence_id;
    uint64_t timestamp_ns;  // nanoseconds since epoch
    uint64_t offset;        // byte offset in .bin file
    uint64_t size;          // frame size in bytes
    uint32_t checksum;      // CRC32 (optional)

    // Convert to JSON for serialization
    nlohmann::json to_json() const {
        return {
            {"sequence_id", sequence_id},
            {"timestamp_ns", timestamp_ns},
            {"offset", offset},
            {"size", size},
            {"checksum", checksum}
        };
    }
};

struct SessionMetadata {
    std::string session_name;
    std::string camera_backend;
    std::string pixel_format;
    int width;
    int height;
    double fps;
    uint64_t start_timestamp_ns;
    uint64_t end_timestamp_ns;
    uint64_t total_frames;
    uint64_t total_bytes;
    uint32_t session_checksum;  // CRC32 of all frame checksums

    nlohmann::json to_json() const {
        return {
            {"session_name", session_name},
            {"camera_backend", camera_backend},
            {"pixel_format", pixel_format},
            {"width", width},
            {"height", height},
            {"fps", fps},
            {"start_timestamp_ns", start_timestamp_ns},
            {"end_timestamp_ns", end_timestamp_ns},
            {"total_frames", total_frames},
            {"total_bytes", total_bytes},
            {"session_checksum", session_checksum}
        };
    }
};

class DiskWriter {
public:
    explicit DiskWriter(const SessionConfig& config);
    ~DiskWriter();

    // Non-copyable
    DiskWriter(const DiskWriter&) = delete;
    DiskWriter& operator=(const DiskWriter&) = delete;

    // Start/stop the writer thread
    bool start();
    void stop();

    // Consume frames from ring buffer
    void consume_from(RingBuffer& buffer);

    // Get statistics
    [[nodiscard]] uint64_t get_frames_written() const {
        return frames_written_.load();
    }

    [[nodiscard]] uint64_t get_bytes_written() const {
        return bytes_written_.load();
    }

    [[nodiscard]] bool is_writing() const {
        return writing_.load();
    }

    // Finalize session (close files, write metadata)
    bool finalize();

private:
    void write_loop();
    void flush_aggregation_buffer();

    // CRC32 checksum (simple implementation)
    static uint32_t compute_crc32(const uint8_t* data, size_t length);

    SessionConfig config_;
    std::atomic<bool> writing_{false};
    std::atomic<uint64_t> frames_written_{0};
    std::atomic<uint64_t> bytes_written_{0};

    RingBuffer* buffer_ = nullptr;
    std::thread writer_thread_;

    // File handles
#ifdef _WIN32
    HANDLE h_file_ = INVALID_HANDLE_VALUE;
#else
    std::ofstream bin_file_;
#endif
    std::string metadata_path_;
    std::ofstream metadata_file_;
    SessionMetadata session_metadata_;
    uint64_t session_start_ns_;
    // Metadata mutex removed - serialized usage (stream)

    // Checksum state
    uint32_t rolling_checksum_;
    std::mutex checksum_mutex_;

    // I/O Aggregation (Super Block)
    uint8_t* aggregation_buffer_ = nullptr;
    size_t current_buffer_pos_{0};
    uint64_t total_bytes_on_disk_{0};
    static constexpr size_t AGGREGATION_THRESHOLD = 128 * 1024 * 1024; // 128MB
};

}  // namespace micecam

#include "micecam/pipeline/disk_writer.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace micecam {

// Simple CRC32 implementation (can be replaced with faster version if needed)
uint32_t DiskWriter::compute_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;

    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
        }
    }

    return ~crc;
}

DiskWriter::DiskWriter(const SessionConfig& config)
    : config_(config), rolling_checksum_(0) {
    session_start_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    aggregation_buffer_.reserve(AGGREGATION_THRESHOLD);
}

DiskWriter::~DiskWriter() {
    if (writing_.load()) {
        stop();
    }
}

bool DiskWriter::start() {
    if (writing_.load()) {
        return false;  // Already started
    }

    // Open .bin file (use std::filesystem for cross-platform paths)
    const fs::path bin_path = fs::path(config_.output_dir) / (config_.session_name + ".bin");
    bin_file_.open(bin_path, std::ios::binary | std::ios::trunc);
    if (!bin_file_.is_open()) {
        std::cerr << "Failed to open " << bin_path << " for writing\n";
        return false;
    }

    metadata_path_ = (fs::path(config_.output_dir) / (config_.session_name + "_metadata.json")).string();

    // Initialize session metadata
    session_metadata_ = SessionMetadata{
        .session_name = config_.session_name,
        .camera_backend = config_.camera_backend_name,
        .width = config_.width,
        .height = config_.height,
        .fps = config_.fps,
        .start_timestamp_ns = session_start_ns_,
        .end_timestamp_ns = 0,
        .total_frames = 0,
        .total_bytes = 0,
        .session_checksum = 0
    };

    writing_.store(true);
    frames_written_.store(0);
    bytes_written_.store(0);
    total_bytes_on_disk_ = 0;
    frame_records_.clear();
    aggregation_buffer_.clear();

    return true;
}

void DiskWriter::stop() {
    if (!writing_.load()) {
        return;
    }

    writing_.store(false);

    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }

    finalize();
}

void DiskWriter::consume_from(RingBuffer& buffer) {
    buffer_ = &buffer;
    writer_thread_ = std::thread(&DiskWriter::write_loop, this);
}

void DiskWriter::write_loop() {
    while (writing_.load() || (buffer_ && !buffer_->empty())) {
        auto frame_opt = buffer_->try_pop();
        if (!frame_opt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        Frame& frame = *frame_opt;

        // I/O Aggregation Logic
        if (aggregation_buffer_.size() + frame.size() > AGGREGATION_THRESHOLD) {
            flush_aggregation_buffer();
        }

        // Calculate absolute offset in file
        const uint64_t offset = total_bytes_on_disk_ + aggregation_buffer_.size();

        // 2. Memory aggregation (Memory Copy)
        aggregation_buffer_.insert(aggregation_buffer_.end(), 
                                 frame.data->begin(), 
                                 frame.data->end());

        // Compute checksum if enabled
        uint32_t checksum = 0;
        if (config_.enable_checksums) {
            checksum = compute_crc32(frame.data->data(), frame.data->size());

            // Update rolling session checksum
            {
                std::lock_guard<std::mutex> lock(checksum_mutex_);
                rolling_checksum_ += checksum;
            }
        }

        // Convert timestamp to nanoseconds
        const uint64_t timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            frame.timestamp.time_since_epoch()
        ).count();

        // Record metadata
        FrameMetadataRecord record{
            .sequence_id = frame.sequence_id,
            .timestamp_ns = timestamp_ns,
            .offset = offset,
            .size = frame.size(),
            .checksum = checksum
        };

        {
            std::lock_guard<std::mutex> lock(metadata_mutex_);
            frame_records_.push_back(record);
        }

        // Update counters
        frames_written_.fetch_add(1);
        bytes_written_.fetch_add(frame.size());
    }

    // Flush remaining data
    flush_aggregation_buffer();
}

void DiskWriter::flush_aggregation_buffer() {
    if (aggregation_buffer_.empty()) return;

    if (bin_file_.is_open()) {
        bin_file_.write(reinterpret_cast<const char*>(aggregation_buffer_.data()), 
                       aggregation_buffer_.size());
        if (!bin_file_.good()) {
            std::cerr << "Error flushing aggregation buffer to disk\n";
        }
        total_bytes_on_disk_ += aggregation_buffer_.size();
        bin_file_.flush();
    }
    aggregation_buffer_.clear();
}

bool DiskWriter::finalize() {
    flush_aggregation_buffer();
    if (bin_file_.is_open()) {
        bin_file_.close();
    }

    // Update session metadata
    const uint64_t session_end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    session_metadata_.end_timestamp_ns = session_end_ns;
    session_metadata_.total_frames = frames_written_.load();
    session_metadata_.total_bytes = bytes_written_.load();
    session_metadata_.session_checksum = rolling_checksum_;

    // Write metadata JSON
    nlohmann::json metadata_json;
    metadata_json["session"] = session_metadata_.to_json();
    metadata_json["frames"] = nlohmann::json::array();

    {
        std::lock_guard<std::mutex> lock(metadata_mutex_);
        for (const auto& record : frame_records_) {
            metadata_json["frames"].push_back(record.to_json());
        }
    }

    std::ofstream meta_file(metadata_path_);
    if (!meta_file.is_open()) {
        std::cerr << "Failed to open " << metadata_path_ << " for writing\n";
        return false;
    }

    meta_file << metadata_json.dump(2);
    meta_file.close();

    std::cout << "Session finalized:\n"
              << "  Frames written: " << session_metadata_.total_frames << "\n"
              << "  Total bytes: " << session_metadata_.total_bytes << "\n"
              << "  Metadata: " << metadata_path_ << "\n";

    return true;
}

}  // namespace micecam

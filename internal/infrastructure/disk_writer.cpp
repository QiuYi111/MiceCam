#include "infrastructure/disk_writer.h"
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
    
#ifdef _WIN32
    // Allocate sector-aligned memory for unbuffered I/O
    aggregation_buffer_ = static_cast<uint8_t*>(VirtualAlloc(NULL, AGGREGATION_THRESHOLD, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
#else
    aggregation_buffer_ = new uint8_t[AGGREGATION_THRESHOLD];
#endif
    current_buffer_pos_ = 0;
}

DiskWriter::~DiskWriter() {
    if (writing_.load()) {
        stop();
    }
#ifdef _WIN32
    if (aggregation_buffer_) VirtualFree(aggregation_buffer_, 0, MEM_RELEASE);
#else
    delete[] aggregation_buffer_;
#endif
}

bool DiskWriter::start() {
    if (writing_.load()) {
        return false;  // Already started
    }

    const fs::path bin_path = fs::path(config_.output_dir) / (config_.session_name + ".bin");
    
#ifdef _WIN32
    h_file_ = CreateFileA(
        bin_path.string().c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        config_.append ? OPEN_ALWAYS : CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
        NULL
    );

    if (h_file_ == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open " << bin_path << " (Error: " << GetLastError() << ")\n";
        return false;
    }

    if (config_.append) {
        // Move to end of file, aligned to 4096
        LARGE_INTEGER li;
        li.QuadPart = 0;
        if (!SetFilePointerEx(h_file_, li, &li, FILE_END)) {
            std::cerr << "Failed to seek to end of file\n";
        }
        total_bytes_on_disk_ = li.QuadPart;
    }
#else
    bin_file_.open(bin_path, std::ios::binary | (config_.append ? std::ios::app : std::ios::trunc));
    if (!bin_file_.is_open()) {
        std::cerr << "Failed to open " << bin_path << " for writing\n";
        return false;
    }
    if (config_.append) {
        bin_file_.seekp(0, std::ios::end);
        total_bytes_on_disk_ = bin_file_.tellp();
    }
#endif

    // Open metadata file (.jsonl) for streaming
    metadata_path_ = (fs::path(config_.output_dir) / (config_.session_name + "_metadata.jsonl")).string();
    metadata_file_.open(metadata_path_, std::ios::out | (config_.append ? std::ios::app : std::ios::trunc));
    if (!metadata_file_.is_open()) {
        std::cerr << "Failed to open " << metadata_path_ << " for writing\n";
        return false;
    }

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

    // Write Session Header
    nlohmann::json header = session_metadata_.to_json();
    header["type"] = "session_start";
    metadata_file_ << header.dump() << "\n";
    metadata_file_.flush();

    writing_.store(true);
    frames_written_.store(0);
    bytes_written_.store(0);
    if (!config_.append) {
        total_bytes_on_disk_ = 0;
    }
    // frame_records_.clear(); removed
    current_buffer_pos_ = 0;

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
    writer_thread_ = std::thread([this]() {
#ifdef _WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
        this->write_loop();
    });
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
        if (current_buffer_pos_ + frame.size() > AGGREGATION_THRESHOLD) {
            flush_aggregation_buffer();
        }

        // Calculate absolute offset in file
        const uint64_t offset = total_bytes_on_disk_ + current_buffer_pos_;

        // Special case: if frame is larger than threshold (unlikely), write it directly (requires alignment)
        if (frame.size() > AGGREGATION_THRESHOLD) {
            // This would need a separate aligned write path, but frames are 2-5MB, threshold is 128MB.
            // For now, let's assume it fits.
        }

        // 2. Memory aggregation (Memory Copy)
        std::memcpy(aggregation_buffer_ + current_buffer_pos_, 
                   frame.data->data(), 
                   frame.data->size());
        current_buffer_pos_ += frame.size();

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

        // Stream metadata to disk
        nlohmann::json j_record = record.to_json();
        j_record["type"] = "frame";
        
        // No mutex needed for stream (single writer thread)
        if (metadata_file_.is_open()) {
            metadata_file_ << j_record.dump() << std::endl; // std::endl flushes
        }

        // Update counters
        frames_written_.fetch_add(1);
        bytes_written_.fetch_add(frame.size());
    }

    // Flush remaining data
    flush_aggregation_buffer();
}

void DiskWriter::flush_aggregation_buffer() {
    if (current_buffer_pos_ == 0) return;

#ifdef _WIN32
    if (h_file_ != INVALID_HANDLE_VALUE) {
        // For unbuffered I/O, we must write in sector-aligned sizes.
        // We round up the write size to the next 4KB boundary if it's the final flush,
        // but for intermediate flushes, we should ideally stay aligned.
        // However, our threshold is 128MB (aligned).
        
        DWORD bytesToWrite = static_cast<DWORD>(current_buffer_pos_);
        // If not aligned to 4096, unbuffered WriteFile will fail.
        // So we PAD the buffer to 4096 alignment if needed.
        size_t remainder = current_buffer_pos_ % 4096;
        if (remainder != 0) {
            size_t padding = 4096 - remainder;
            std::memset(aggregation_buffer_ + current_buffer_pos_, 0, padding);
            bytesToWrite += static_cast<DWORD>(padding);
        }

        DWORD bytesWritten;
        if (!WriteFile(h_file_, aggregation_buffer_, bytesToWrite, &bytesWritten, NULL)) {
            std::cerr << "Error flushing aggregation buffer to disk (Unbuffered WriteFile failed: " << GetLastError() << ")\n";
        }
        total_bytes_on_disk_ += bytesWritten;
    }
#else
    if (bin_file_.is_open()) {
        bin_file_.write(reinterpret_cast<const char*>(aggregation_buffer_), 
                       current_buffer_pos_);
        if (!bin_file_.good()) {
            std::cerr << "Error flushing aggregation buffer to disk\n";
        }
        total_bytes_on_disk_ += current_buffer_pos_;
        bin_file_.flush();
    }
#endif
    current_buffer_pos_ = 0;
}

bool DiskWriter::finalize() {
    flush_aggregation_buffer();
#ifdef _WIN32
    if (h_file_ != INVALID_HANDLE_VALUE) {
        CloseHandle(h_file_);
        h_file_ = INVALID_HANDLE_VALUE;
    }
#else
    if (bin_file_.is_open()) {
        bin_file_.close();
    }
#endif

    // Update session metadata
    const uint64_t session_end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    session_metadata_.end_timestamp_ns = session_end_ns;
    session_metadata_.total_frames = frames_written_.load();
    session_metadata_.total_bytes = bytes_written_.load();
    session_metadata_.session_checksum = rolling_checksum_;

    // Write Session Footer
    nlohmann::json footer = session_metadata_.to_json();
    footer["type"] = "session_end";
    
    if (metadata_file_.is_open()) {
        metadata_file_ << footer.dump() << "\n";
        metadata_file_.close();
    }

    // No bulk JSON array write anymore

    std::cout << "Session finalized:\n"
              << "  Frames written: " << session_metadata_.total_frames << "\n"
              << "  Total bytes: " << session_metadata_.total_bytes << "\n"
              << "  Metadata: " << metadata_path_ << "\n";

    return true;
}

}  // namespace micecam

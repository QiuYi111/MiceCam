#include "infrastructure/SRTWriter.h"

#include <spdlog/spdlog.h>

#include <cinttypes>
#include <cstdio>

namespace micecam::infrastructure {

static void format_srt_time(uint64_t total_us, char* buf, size_t bufsz) {
    uint64_t hours = total_us / 3600000000ULL;
    uint64_t remaining = total_us % 3600000000ULL;
    uint64_t minutes = remaining / 60000000ULL;
    remaining %= 60000000ULL;
    uint64_t seconds = remaining / 1000000ULL;
    uint64_t millis = (remaining % 1000000ULL) / 1000ULL;
    snprintf(buf, bufsz, "%02" PRIu64 ":%02" PRIu64 ":%02" PRIu64 ",%03" PRIu64,
             hours, minutes, seconds, millis);
}

SRTWriter::SRTWriter() = default;

SRTWriter::~SRTWriter() {
    close();
}

bool SRTWriter::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    file_ = fopen(path.c_str(), "w");
    if (!file_) {
        spdlog::error("Failed to open SRT file: {}", path);
        return false;
    }

    entry_count_ = 0;
    spdlog::info("SRTWriter opened: {}", path);
    return true;
}

void SRTWriter::write_entry(uint64_t seq, const domain::FrameTimestamp& ts, bool skipped) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!file_) return;

    entry_count_++;

    uint64_t offset_us = ts.session_offset_us;
    uint64_t frame_duration_us = 33333;

    char start_time[32];
    char end_time[32];
    format_srt_time(offset_us, start_time, sizeof(start_time));
    format_srt_time(offset_us + frame_duration_us, end_time, sizeof(end_time));

    fprintf(file_, "%" PRIu64 "\n", entry_count_);
    fprintf(file_, "%s --> %s\n", start_time, end_time);
    fprintf(file_, "seq=%" PRIu64 " offset_us=%" PRIu64 " skipped=%s\n",
            seq, offset_us, skipped ? "true" : "false");
    fprintf(file_, "\n");

    fflush(file_);
}

void SRTWriter::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_) {
        fclose(file_);
        file_ = nullptr;
        spdlog::info("SRTWriter closed: {} entries written", entry_count_);
    }
}

} // namespace micecam::infrastructure

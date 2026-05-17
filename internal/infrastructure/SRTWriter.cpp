#include "infrastructure/SRTWriter.h"

#include <spdlog/spdlog.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>

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

std::string SRTWriter::wall_time_to_iso8601(uint64_t wall_time_ns) {
    if (wall_time_ns == 0) return "";

    auto total_sec = static_cast<time_t>(wall_time_ns / 1000000000ULL);
    uint64_t remaining_ns = wall_time_ns % 1000000000ULL;
    uint64_t microseconds = remaining_ns / 1000ULL;

    struct tm tm_buf;
    localtime_r(&total_sec, &tm_buf);

    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);

    char result[96];
    snprintf(result, sizeof(result), "%s.%06" PRIu64, time_buf, microseconds);
    return std::string(result);
}

SRTWriter::SRTWriter() = default;

SRTWriter::~SRTWriter() {
    close();
}

bool SRTWriter::open(const std::string& path, double fps) {
    std::lock_guard<std::mutex> lock(mutex_);

    file_ = fopen(path.c_str(), "w");
    if (!file_) {
        spdlog::error("Failed to open SRT file: {}", path);
        return false;
    }

    entry_count_ = 0;
    fps_ = fps;
    spdlog::info("SRTWriter opened: {}", path);
    return true;
}

void SRTWriter::write_entry(uint64_t seq, const domain::FrameTimestamp& ts, bool skipped) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!file_) return;

    entry_count_++;

    uint64_t offset_us = ts.session_offset_us;
    uint64_t frame_duration_us = static_cast<uint64_t>(1000000.0 / fps_ + 0.5);

    char start_time[32];
    char end_time[32];
    format_srt_time(offset_us, start_time, sizeof(start_time));
    format_srt_time(offset_us + frame_duration_us, end_time, sizeof(end_time));

    fprintf(file_, "%" PRIu64 "\n", entry_count_);
    fprintf(file_, "%s --> %s\n", start_time, end_time);
    fprintf(file_, "seq=%" PRIu64 " offset_us=%" PRIu64 " skipped=%s\n",
            seq, offset_us, skipped ? "true" : "false");

    if (ts.wall_time_ns != 0) {
        std::string wt = wall_time_to_iso8601(ts.wall_time_ns);
        if (!wt.empty()) {
            fprintf(file_, "wall_time=%s\n", wt.c_str());
        }
    }

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

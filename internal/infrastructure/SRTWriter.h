#pragma once

#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>

#include "domain/FrameTimestamp.h"

namespace micecam::infrastructure {

class SRTWriter {
public:
    SRTWriter();
    ~SRTWriter();

    SRTWriter(const SRTWriter&) = delete;
    SRTWriter& operator=(const SRTWriter&) = delete;

    bool open(const std::string& path, double fps = 30.0);
    void write_entry(uint64_t seq, const domain::FrameTimestamp& ts, bool skipped);
    void close();

private:
    std::mutex mutex_;
    std::FILE* file_ = nullptr;
    uint64_t entry_count_ = 0;
    double fps_ = 30.0;

    static std::string wall_time_to_iso8601(uint64_t wall_time_ns);
};

} // namespace micecam::infrastructure

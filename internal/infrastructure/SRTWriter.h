#pragma once

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

    bool open(const std::string& path);
    void write_entry(uint64_t seq, const domain::FrameTimestamp& ts, bool skipped);
    void close();

private:
    std::mutex mutex_;
    std::FILE* file_ = nullptr;
    uint64_t entry_count_ = 0;
};

} // namespace micecam::infrastructure

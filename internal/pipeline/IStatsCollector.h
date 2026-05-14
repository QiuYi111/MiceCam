#pragma once

#include <cstdint>

#include "domain/StreamStats.h"
#include "domain/AlertRecord.h"

namespace micecam::pipeline {

class IStatsCollector {
public:
    virtual ~IStatsCollector() = default;

    virtual void record_frame(uint64_t expected_seq, uint64_t actual_seq,
                              double encode_latency_us, uint64_t frame_interval_us) = 0;
    virtual domain::StreamStats finalize() = 0;
    virtual void add_alert(const domain::AlertRecord& alert) = 0;
};

} // namespace micecam::pipeline

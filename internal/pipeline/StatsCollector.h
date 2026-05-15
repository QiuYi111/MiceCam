#pragma once

#include <algorithm>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include "IStatsCollector.h"
#include "domain/AlertRecord.h"
#include "domain/StreamStats.h"

namespace micecam::pipeline {

class StatsCollector : public IStatsCollector {
public:
    explicit StatsCollector(const std::string& stream_id);
    ~StatsCollector() override = default;

    void start(uint64_t expected_frame_interval_us);
    void record_frame(uint64_t expected_seq, uint64_t actual_seq,
                      double encode_latency_us, uint64_t frame_interval_us) override;
    void add_bytes(uint64_t bytes);
    void set_encoder(std::string encoder_name, bool fallback);
    domain::StreamStats snapshot();
    domain::StreamStats finalize() override;
    void add_alert(const domain::AlertRecord& alert) override;

private:
    std::mutex mutex_;
    std::string stream_id_;
    uint64_t frames_expected_ = 0;
    uint64_t frames_actual_ = 0;
    double encode_sum_us_ = 0.0;
    double encode_min_us_ = 1e18;
    double encode_max_us_ = 0.0;
    double frame_interval_sum_us_ = 0.0;
    double frame_interval_max_deviation_us_ = 0.0;
    uint64_t expected_frame_interval_us_ = 0;
    int encode_count_ = 0;
    uint64_t bytes_written_ = 0;
    std::string encoder_used_;
    bool encoder_fallback_ = false;
    std::vector<domain::AlertRecord> alerts_;
};

} // namespace micecam::pipeline

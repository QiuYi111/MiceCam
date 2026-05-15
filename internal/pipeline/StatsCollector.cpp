#include "StatsCollector.h"

#include <cmath>

namespace micecam::pipeline {

StatsCollector::StatsCollector(const std::string& stream_id)
    : stream_id_(stream_id) {}

void StatsCollector::start(uint64_t expected_frame_interval_us) {
    expected_frame_interval_us_ = expected_frame_interval_us;
}

void StatsCollector::record_frame(uint64_t /*expected_seq*/, uint64_t /*actual_seq*/,
                                   double encode_latency_us, uint64_t frame_interval_us) {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_expected_++;
    frames_actual_++;
    encode_sum_us_ += encode_latency_us;
    encode_min_us_ = std::min(encode_min_us_, encode_latency_us);
    encode_max_us_ = std::max(encode_max_us_, encode_latency_us);
    frame_interval_sum_us_ += static_cast<double>(frame_interval_us);
    encode_count_++;

    double deviation = std::abs(static_cast<double>(frame_interval_us) -
                                static_cast<double>(expected_frame_interval_us_));
    frame_interval_max_deviation_us_ = std::max(frame_interval_max_deviation_us_, deviation);
}

void StatsCollector::add_bytes(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    bytes_written_ += bytes;
}

void StatsCollector::set_encoder(std::string encoder_name, bool fallback) {
    std::lock_guard<std::mutex> lock(mutex_);
    encoder_used_ = std::move(encoder_name);
    encoder_fallback_ = fallback;
}

domain::StreamStats StatsCollector::snapshot() {
    domain::StreamStats stats;
    stats.stream_id = stream_id_;
    stats.frames_expected = frames_expected_;
    stats.frames_actual = frames_actual_;
    stats.drop_rate = (frames_expected_ > 0)
        ? static_cast<double>(frames_expected_ - frames_actual_) / frames_expected_
        : 0.0;
    stats.avg_encode_latency_us = (encode_count_ > 0) ? encode_sum_us_ / encode_count_ : 0.0;
    stats.max_encode_latency_us = encode_max_us_;
    stats.min_encode_latency_us = (encode_min_us_ > 1e17) ? 0.0 : encode_min_us_;
    stats.avg_frame_interval_us = (encode_count_ > 0) ? frame_interval_sum_us_ / encode_count_ : 0.0;
    stats.max_frame_interval_deviation_us = frame_interval_max_deviation_us_;
    stats.bytes_written = bytes_written_;
    stats.encoder_used = encoder_used_;
    stats.encoder_fallback = encoder_fallback_;
    return stats;
}

domain::StreamStats StatsCollector::finalize() {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot();
}

void StatsCollector::add_alert(const domain::AlertRecord& alert) {
    std::lock_guard<std::mutex> lock(mutex_);
    alerts_.push_back(alert);
}

} // namespace micecam::pipeline

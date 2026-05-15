#include "infrastructure/PluginStreamConsumer.h"

#include <spdlog/spdlog.h>

#include <chrono>

#include "infrastructure/PluginRingReader.h"
#include "pipeline/RecordingPipeline.h"

namespace micecam::infrastructure {

PluginStreamConsumer::PluginStreamConsumer(pipeline::RecordingPipeline& pipeline,
                                           const PluginStreamConfig& config)
    : pipeline_(pipeline), config_(config) {}

PluginStreamConsumer::~PluginStreamConsumer() {
    stop();
}

bool PluginStreamConsumer::start() {
    if (running_.load()) return false;

    reader_ = std::make_unique<PluginRingReader>();
    if (!reader_->open(config_.shm_name)) {
        spdlog::error("PluginStreamConsumer: failed to open ring {}", config_.shm_name);
        reader_.reset();
        return false;
    }

    running_.store(true);
    consumer_thread_ = std::jthread([this](std::stop_token) { consumerLoop(); });

    spdlog::info("PluginStreamConsumer started: plugin={} stream={}",
                 config_.plugin_id, config_.stream_id);
    return true;
}

void PluginStreamConsumer::stop() {
    if (!running_.load()) return;

    running_.store(false);
    if (consumer_thread_.joinable()) {
        consumer_thread_.request_stop();
        consumer_thread_.join();
    }

    if (reader_) {
        reader_->close();
        reader_.reset();
    }

    spdlog::info("PluginStreamConsumer stopped: plugin={} stream={}",
                 config_.plugin_id, config_.stream_id);
}

void PluginStreamConsumer::consumerLoop() {
    while (running_.load()) {
        ReadSlotData slot;
        if (!reader_->readNextFrame(slot, 100)) {
            continue;
        }

        auto reader_stats = reader_->stats();

        uint64_t drop_delta = reader_stats.total_drops - last_reader_drops_;
        uint64_t bp_delta = reader_stats.backpressure_events - last_reader_bp_events_;
        last_reader_drops_ = reader_stats.total_drops;
        last_reader_bp_events_ = reader_stats.backpressure_events;

        std::string source_format = payloadKindToSourceFormat(slot.payload_kind);

        pipeline::FrameData frame;
        frame.stream_id = config_.stream_id;
        frame.data = slot.data.data();
        frame.size = slot.data.size();
        frame.width = slot.width;
        frame.height = slot.height;
        frame.pts = static_cast<int64_t>(slot.timestamp_ns / 1000);
        frame.source_format = source_format;

        pipeline_.push_frame(frame);

        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            transport_stats_.frames_read++;
            transport_stats_.frames_dropped += drop_delta;
            transport_stats_.backpressure_events += bp_delta;

            double lag = static_cast<double>(reader_stats.current_lag);
            lag_sample_count_++;
            lag_sum_ += lag;
            if (lag > lag_max_) lag_max_ = lag;
            transport_stats_.avg_consumer_lag = lag_sum_ / static_cast<double>(lag_sample_count_);
            transport_stats_.max_consumer_lag = lag_max_;
        }
    }
}

std::string PluginStreamConsumer::payloadKindToSourceFormat(domain::PayloadKind kind) const {
    switch (kind) {
        case domain::PayloadKind::RAW:
            return "raw_rgb24";
        case domain::PayloadKind::MJPEG:
            return "mjpeg";
        case domain::PayloadKind::H264:
            return "h264";
        case domain::PayloadKind::H265:
            return "h265";
        default:
            return "raw_rgb24";
    }
}

TransportStats PluginStreamConsumer::getTransportStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return transport_stats_;
}

PluginSourceInfo PluginStreamConsumer::getPluginSourceInfo() const {
    PluginSourceInfo info;
    info.plugin_id = config_.plugin_id;
    info.device_id = config_.device_id;
    info.transport = "posix_shm";
    info.ring_slot_count = config_.slot_count;
    info.ring_slot_size = config_.slot_size;
    return info;
}

} // namespace micecam::infrastructure

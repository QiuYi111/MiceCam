#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "domain/StreamRingDescriptor.h"
#include "infrastructure/SharedMemoryBackend.h"

namespace micecam::pipeline {
class RecordingPipeline;
}

namespace micecam::infrastructure {

class PluginRingReader;
class StreamLivenessMonitor;

struct PluginStreamConfig {
    std::string plugin_id;
    std::string device_id;
    std::string stream_id;
    std::string shm_name;
    uint32_t slot_count = 8;
    uint32_t slot_size = 4194304;
};

struct TransportStats {
    uint64_t frames_read = 0;
    uint64_t frames_dropped = 0;
    uint64_t backpressure_events = 0;
    double avg_consumer_lag = 0.0;
    double max_consumer_lag = 0.0;
};

struct PluginSourceInfo {
    std::string plugin_id;
    std::string device_id;
    std::string transport = kShmTransportType;
    uint32_t ring_slot_count = 0;
    uint32_t ring_slot_size = 0;
};

class PluginStreamConsumer {
public:
    PluginStreamConsumer(pipeline::RecordingPipeline& pipeline,
                         const PluginStreamConfig& config);
    ~PluginStreamConsumer();

    PluginStreamConsumer(const PluginStreamConsumer&) = delete;
    PluginStreamConsumer& operator=(const PluginStreamConsumer&) = delete;

    bool start();
    void stop();
    void set_liveness_monitor(StreamLivenessMonitor* monitor);
    TransportStats getTransportStats() const;
    PluginSourceInfo getPluginSourceInfo() const;

private:
    void consumerLoop();
    std::string payloadKindToSourceFormat(domain::PayloadKind kind) const;

    pipeline::RecordingPipeline& pipeline_;
    PluginStreamConfig config_;

    StreamLivenessMonitor* monitor_ = nullptr;
    std::unique_ptr<PluginRingReader> reader_;
    std::jthread consumer_thread_;
    std::atomic<bool> running_{false};

    mutable std::mutex stats_mutex_;
    TransportStats transport_stats_;
    uint64_t last_reader_drops_ = 0;
    uint64_t last_reader_bp_events_ = 0;
    uint64_t lag_sample_count_ = 0;
    double lag_sum_ = 0.0;
    double lag_max_ = 0.0;
};

} // namespace micecam::infrastructure

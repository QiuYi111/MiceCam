#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "IStatsCollector.h"
#include "domain/CalibrationResult.h"
#include "domain/EncoderConfig.h"
#include "domain/SessionMetadata.h"
#include "domain/StreamConfig.h"
#include "domain/StreamStats.h"
#include "domain/FrameTimestamp.h"

namespace micecam::infrastructure {
class StreamWriter;
class SRTWriter;
class MetadataWriter;
class Watchdog;
class AlertManager;
}

namespace micecam::pipeline {

class TranscodeStage;
class StatsCollector;

enum class PayloadKind {
    RAW = 0,
    MJPEG = 1,
    H264 = 2,
    H265 = 3,
};

struct SessionConfig {
    std::string session_id;
    std::string output_dir = ".";
    std::vector<domain::StreamConfig> streams;
    domain::EncoderConfig encoder;
    int watchdog_timeout_s = 3;
    std::unordered_map<std::string, domain::CalibrationResult> calibration_results;
};

struct FrameData {
    std::string stream_id;
    const uint8_t* data = nullptr;
    size_t size = 0;
    int width = 0;
    int height = 0;
    int64_t pts = 0;
    std::string source_format = "rgb24";
    PayloadKind payload_kind = PayloadKind::RAW;
    bool is_keyframe = false;
    uint64_t dropped_frame_count = 0;
};

enum class PipelineState {
    IDLE,
    RUNNING,
    STOPPING,
    FINALIZED
};

struct StreamPipeline {
    std::unique_ptr<TranscodeStage> transcoder;
    std::unique_ptr<infrastructure::StreamWriter> writer;
    std::unique_ptr<infrastructure::SRTWriter> srt;
    std::unique_ptr<StatsCollector> stats;
    std::string stream_id;
    std::string output_prefix;
    int width = 0;
    int height = 0;
    int fps = 30;
    uint64_t frame_seq = 0;
    bool initialized = false;
    int fallback_gop_size = 60;
    uint64_t overflow_count = 0;
};

class RecordingPipeline {
public:
    RecordingPipeline();
    ~RecordingPipeline();

    RecordingPipeline(const RecordingPipeline&) = delete;
    RecordingPipeline& operator=(const RecordingPipeline&) = delete;

    bool start(const SessionConfig& config);
    bool push_frame(const FrameData& frame);
    void stop();
    std::pair<domain::SessionMetadata, std::vector<domain::StreamStats>> result();

    void set_watchdog(infrastructure::Watchdog* wd) { watchdog_ = wd; }
    void set_alert_manager(infrastructure::AlertManager* am) { alert_mgr_ = am; }
    void set_plugin_source(const nlohmann::json& plugin_source);
    void set_stream_transport_stats(const std::string& stream_id, const nlohmann::json& transport);

    uint64_t get_overflow_count(const std::string& stream_id) const;

    bool finalize_stream(const std::string& stream_id);
    bool start_reconnect(const std::string& stream_id, int reconnect_index);

private:
    bool create_stream_pipeline(const domain::StreamConfig& sc,
                                const std::string& output_dir,
                                const domain::EncoderConfig& enc_cfg);

    std::mutex mutex_;
    std::atomic<PipelineState> state_{PipelineState::IDLE};
    SessionConfig config_;
    std::unordered_map<std::string, std::unique_ptr<StreamPipeline>> streams_;
    infrastructure::Watchdog* watchdog_ = nullptr;
    infrastructure::AlertManager* alert_mgr_ = nullptr;
    nlohmann::json plugin_source_;
    std::unordered_map<std::string, nlohmann::json> stream_transport_stats_;
};

} // namespace micecam::pipeline

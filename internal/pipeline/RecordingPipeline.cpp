#include "RecordingPipeline.h"

#include <cinttypes>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

#include "TranscodeStage.h"
#include "StatsCollector.h"
#include "infrastructure/StreamWriter.h"
#include "infrastructure/SRTWriter.h"
#include "infrastructure/MetadataWriter.h"
#include "infrastructure/Watchdog.h"
#include "infrastructure/AlertManager.h"
#include "domain/AlertRecord.h"

namespace micecam::pipeline {

RecordingPipeline::RecordingPipeline() = default;

RecordingPipeline::~RecordingPipeline() {
    if (state_ == PipelineState::RUNNING) {
        stop();
    }
}

bool RecordingPipeline::start(const SessionConfig& config) {
    if (state_ != PipelineState::IDLE) return false;

    config_ = config;
    std::string output_dir = config.output_dir;
    if (output_dir.empty()) output_dir = ".";
    output_dir += "/" + config.session_id;

    std::error_code ec;
    std::filesystem::create_directories(output_dir, ec);
    if (ec) return false;

    for (size_t i = 0; i < config.streams.size(); i++) {
        auto& sc = config.streams[i];
        if (!create_stream_pipeline(sc, output_dir, config.encoder)) {
            return false;
        }
    }

    state_ = PipelineState::RUNNING;

    if (watchdog_) {
        watchdog_->start();
    }

    return true;
}

bool RecordingPipeline::create_stream_pipeline(const domain::StreamConfig& sc,
                                                const std::string& output_dir,
                                                const domain::EncoderConfig& enc_cfg) {
    auto sp = std::make_unique<StreamPipeline>();
    sp->stream_id = sc.device_id + "_" + std::to_string(sc.stream_index);
    sp->width = sc.width > 0 ? sc.width : 1920;
    sp->height = sc.height > 0 ? sc.height : 1080;
    sp->fps = sc.framerate > 0 ? sc.framerate : 30;
    sp->output_prefix = output_dir + "/" + sp->stream_id;

    auto cal_it = config_.calibration_results.find(sp->stream_id);
    if (cal_it != config_.calibration_results.end() && cal_it->second.success && cal_it->second.min_gop > 0) {
        sp->fallback_gop_size = cal_it->second.min_gop;
    }

    sp->transcoder = std::make_unique<TranscodeStage>();
    if (!sp->transcoder->initialize(enc_cfg)) {
        return false;
    }

    sp->writer = std::make_unique<infrastructure::StreamWriter>();
    std::string mp4_path = sp->output_prefix + ".mp4";
    if (!sp->writer->open(mp4_path, sp->width, sp->height, sp->fps)) {
        return false;
    }

    sp->srt = std::make_unique<infrastructure::SRTWriter>();
    std::string srt_path = sp->output_prefix + ".srt";
    sp->srt->open(srt_path);

    sp->stats = std::make_unique<StatsCollector>(sp->stream_id);
    uint64_t frame_interval_us = 1000000ULL / sp->fps;
    sp->stats->start(frame_interval_us);

    sp->initialized = true;
    streams_[sp->stream_id] = std::move(sp);
    return true;
}

bool RecordingPipeline::push_frame(const FrameData& frame) {
    if (state_ != PipelineState::RUNNING) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = streams_.find(frame.stream_id);
    if (it == streams_.end()) return false;

    auto& sp = *it->second;
    if (!sp.initialized) return false;

    if (frame.dropped_frame_count > 0) {
        sp.overflow_count += frame.dropped_frame_count;
        spdlog::warn("Stream {} ring buffer overflow: {} frames dropped", frame.stream_id, frame.dropped_frame_count);
    }

    uint64_t frame_seq = sp.frame_seq++;
    uint64_t frame_interval_us = 1000000ULL / sp.fps;

    if (frame.payload_kind == PayloadKind::H264 || frame.payload_kind == PayloadKind::H265) {
        if (frame.data && frame.size > 0) {
            sp.stats->set_encoder(frame.payload_kind == PayloadKind::H264 ? "h264" : "h265", false);
            sp.stats->record_frame(frame_seq, frame_seq, 0, frame_interval_us);
            sp.writer->write_packet(frame.data, frame.size, frame.pts, frame.pts, frame.is_keyframe);
            sp.stats->add_bytes(frame.size);
        }
    } else {
        auto packet = sp.transcoder->process(frame.data, frame.size, frame.width, frame.height, frame.pts, frame.source_format);
        if (!packet.empty()) {
            sp.stats->set_encoder(sp.transcoder->encoder_name(), false);
            sp.stats->record_frame(frame_seq, frame_seq, 0, frame_interval_us);

            bool keyframe = (frame_seq % static_cast<uint64_t>(sp.fallback_gop_size) == 0);
            sp.writer->write_packet(packet.data(), packet.size(), frame.pts, frame.pts, keyframe);
            sp.stats->add_bytes(packet.size());
        }
    }

    domain::FrameTimestamp fts;
    fts.session_offset_us = static_cast<uint64_t>(frame.pts);
    sp.srt->write_entry(frame_seq, fts, false);

    if (watchdog_) {
        watchdog_->feed();
    }

    return true;
}

void RecordingPipeline::stop() {
    if (state_ != PipelineState::RUNNING) return;
    state_ = PipelineState::STOPPING;

    if (watchdog_) {
        watchdog_->stop();
    }

    for (auto& [id, sp] : streams_) {
        if (!sp->initialized) continue;

        std::vector<uint8_t> flushed;
        if (sp->transcoder && sp->transcoder->flush(flushed) && !flushed.empty()) {
            const int64_t pts = static_cast<int64_t>(sp->frame_seq);
            if (sp->writer->write_packet(flushed.data(), flushed.size(), pts, pts, true)) {
                sp->stats->add_bytes(flushed.size());
            }
        }

        sp->writer->close();
        sp->srt->close();
    }

    state_ = PipelineState::FINALIZED;
}

void RecordingPipeline::set_plugin_source(const nlohmann::json& plugin_source) {
    std::lock_guard<std::mutex> lock(mutex_);
    plugin_source_ = plugin_source;
}

void RecordingPipeline::set_stream_transport_stats(const std::string& stream_id, const nlohmann::json& transport) {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_transport_stats_[stream_id] = transport;
}

uint64_t RecordingPipeline::get_overflow_count(const std::string& stream_id) const {
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) return 0;
    return it->second->overflow_count;
}

bool RecordingPipeline::finalize_stream(const std::string& stream_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(stream_id);
    if (it == streams_.end() || !it->second->initialized) return false;

    auto& sp = *it->second;

    std::vector<uint8_t> flushed;
    if (sp.transcoder && sp.transcoder->flush(flushed) && !flushed.empty()) {
        const int64_t pts = static_cast<int64_t>(sp.frame_seq);
        if (sp.writer->write_packet(flushed.data(), flushed.size(), pts, pts, true)) {
            sp.stats->add_bytes(flushed.size());
        }
    }

    sp.writer->close();
    sp.srt->close();
    sp.initialized = false;

    spdlog::info("Finalized stream {} after plugin crash", stream_id);
    return true;
}

bool RecordingPipeline::start_reconnect(const std::string& stream_id, int reconnect_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) return false;

    auto& sp = *it->second;
    sp.output_prefix = sp.output_prefix + "_reconnect_" + std::to_string(reconnect_index);

    sp.writer = std::make_unique<infrastructure::StreamWriter>();
    std::string mp4_path = sp.output_prefix + ".mp4";
    if (!sp.writer->open(mp4_path, sp.width, sp.height, sp.fps)) {
        spdlog::error("Failed to open reconnect MP4 for stream {}", stream_id);
        return false;
    }

    sp.srt = std::make_unique<infrastructure::SRTWriter>();
    std::string srt_path = sp.output_prefix + ".srt";
    sp.srt->open(srt_path);

    sp.initialized = true;

    auto now = std::chrono::system_clock::now();
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();

    std::string reconnect_meta = sp.output_prefix + "_meta.json";
    nlohmann::json meta_j;
    meta_j["stream_id"] = stream_id;
    meta_j["reconnect_index"] = reconnect_index;

    auto total_sec = static_cast<time_t>(now_ns / 1000000000ULL);
    uint64_t remaining_ns = now_ns % 1000000000ULL;
    uint64_t microseconds = remaining_ns / 1000ULL;
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &total_sec);
#else
    localtime_r(&total_sec, &tm_buf);
#endif
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);
    char iso_buf[96];
    snprintf(iso_buf, sizeof(iso_buf), "%s.%06" PRIu64, time_buf, microseconds);
    meta_j["crash_recovery_wall_time"] = std::string(iso_buf);

    std::ofstream out(reconnect_meta);
    if (out.is_open()) {
        out << meta_j.dump(2);
    }

    spdlog::info("Started reconnect recording for stream {} -> _reconnect_{}.mp4",
                 stream_id, reconnect_index);
    return true;
}

std::pair<domain::SessionMetadata, std::vector<domain::StreamStats>>
RecordingPipeline::result() {
    domain::SessionMetadata meta;
    meta.session_id = config_.session_id;
    meta.output_dir = config_.output_dir;
    meta.encoder_name = streams_.empty() ? "" : streams_.begin()->second->transcoder->encoder_name();
    meta.bitrate_kbps = config_.encoder.bitrate_kbps;
    meta.keyframe_interval = config_.encoder.keyframe_interval;
    meta.plugin_source = plugin_source_;

    std::vector<domain::StreamStats> stats_list;
    for (auto& [id, sp] : streams_) {
        if (sp->initialized) {
            auto stats = sp->stats->finalize();
            auto tr_it = stream_transport_stats_.find(id);
            if (tr_it != stream_transport_stats_.end()) {
                stats.transport = tr_it->second;
            }
            stats_list.push_back(stats);
            meta.stream_configs.push_back(domain::StreamConfig{
                id, 0, sp->width, sp->height, sp->fps, ""
            });
        }
    }

    std::string meta_path = config_.output_dir + "/" + config_.session_id + "/_meta.json";
    infrastructure::MetadataWriter::write_session_header(meta, meta_path);

    std::string stats_path = config_.output_dir + "/" + config_.session_id + "/_stats.json";
    infrastructure::MetadataWriter::write_stats(stats_path, stats_list);

    return {meta, stats_list};
}

} // namespace micecam::pipeline

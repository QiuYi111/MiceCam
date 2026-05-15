#include "RecordingPipeline.h"

#include <chrono>
#include <filesystem>
#include <sstream>

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

    uint64_t frame_seq = sp.frame_seq++;
    uint64_t frame_interval_us = 1000000ULL / sp.fps;

    auto packet = sp.transcoder->process(frame.data, frame.size, frame.width, frame.height, frame.pts, frame.source_format);
    if (!packet.empty()) {
        sp.stats->set_encoder(sp.transcoder->encoder_name(), false);
        sp.stats->record_frame(frame_seq, frame_seq, 0, frame_interval_us);

        bool keyframe = (frame_seq % static_cast<uint64_t>(config_.encoder.keyframe_interval) == 0);
        sp.writer->write_packet(packet.data(), packet.size(), frame.pts, frame.pts, keyframe);
        sp.stats->add_bytes(packet.size());
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

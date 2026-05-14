#include "pipeline/TranscodeStage.h"

#include <spdlog/spdlog.h>

#include <cstring>

#include "infrastructure/FFmpegEncoder.h"

namespace micecam::pipeline {

TranscodeStage::TranscodeStage() = default;
TranscodeStage::~TranscodeStage() = default;

bool TranscodeStage::initialize(const domain::EncoderConfig& config) {
    encoder_ = std::make_unique<infrastructure::FFmpegEncoder>();
    return encoder_->initialize(config);
}

std::vector<uint8_t> TranscodeStage::process(const uint8_t* data, size_t size, int width, int height, int64_t pts, const std::string& source_format) {
    if (source_format == "h264" || source_format == "H264") {
        return std::vector<uint8_t>(data, data + size);
    }

    if (!encoder_) {
        return {};
    }

    return encoder_->encode(data, width, height, pts);
}

bool TranscodeStage::flush(std::vector<uint8_t>& out) {
    if (!encoder_) return false;
    return encoder_->flush(out);
}

std::string TranscodeStage::encoder_name() const {
    if (!encoder_) return "";
    return encoder_->encoder_name();
}

} // namespace micecam::pipeline

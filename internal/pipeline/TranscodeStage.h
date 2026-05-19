#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "domain/EncoderConfig.h"

namespace micecam::infrastructure {
class FFmpegEncoder;
}

namespace micecam::pipeline {

class TranscodeStage {
public:
    TranscodeStage();
    ~TranscodeStage();

    TranscodeStage(const TranscodeStage&) = delete;
    TranscodeStage& operator=(const TranscodeStage&) = delete;

    bool initialize(const domain::EncoderConfig& config);
    std::vector<uint8_t> process(const uint8_t* data, size_t size, int width, int height, int64_t pts, const std::string& source_format);
    bool flush(std::vector<uint8_t>& out);
    std::string encoder_name() const;

private:
    std::unique_ptr<infrastructure::FFmpegEncoder> encoder_;
};

} // namespace micecam::pipeline

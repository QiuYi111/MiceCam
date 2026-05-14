#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pipeline/IEncoder.h"

struct AVCodecContext;
struct AVCodec;
struct SwsContext;

namespace micecam::infrastructure {

class FFmpegEncoder : public pipeline::IEncoder {
public:
    FFmpegEncoder();
    ~FFmpegEncoder() override;

    FFmpegEncoder(const FFmpegEncoder&) = delete;
    FFmpegEncoder& operator=(const FFmpegEncoder&) = delete;

    bool initialize(const domain::EncoderConfig& config) override;
    std::vector<uint8_t> encode(const uint8_t* rgb_data, int width, int height, int64_t pts) override;
    bool flush(std::vector<uint8_t>& out) override;
    std::string encoder_name() const override;

private:
    bool ensure_context(int width, int height);

    AVCodecContext* enc_ctx_ = nullptr;
    const AVCodec* active_codec_ = nullptr;
    SwsContext* sws_ctx_ = nullptr;
    int enc_width_ = 0;
    int enc_height_ = 0;
    int sws_src_width_ = 0;
    int sws_src_height_ = 0;
    std::string active_encoder_;
    domain::EncoderConfig stored_config_;
};

} // namespace micecam::infrastructure

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "internal/domain/EncoderConfig.h"

namespace micecam::pipeline {

class IEncoder {
public:
    virtual ~IEncoder() = default;

    virtual bool initialize(const domain::EncoderConfig& config) = 0;
    virtual std::vector<uint8_t> encode(const uint8_t* data, size_t size, int64_t pts) = 0;
    virtual bool flush(std::vector<uint8_t>& out) = 0;
    virtual std::string encoder_name() const = 0;
};

} // namespace micecam::pipeline

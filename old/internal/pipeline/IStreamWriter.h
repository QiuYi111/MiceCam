#pragma once

#include <cstdint>
#include <string>

namespace micecam::pipeline {

class IStreamWriter {
public:
    virtual ~IStreamWriter() = default;

    virtual bool open(const std::string& path, int width, int height, int fps) = 0;
    virtual bool write_packet(const uint8_t* data, size_t size, int64_t pts, int64_t dts, bool keyframe) = 0;
    virtual bool close() = 0;
};

} // namespace micecam::pipeline

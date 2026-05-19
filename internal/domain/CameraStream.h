#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace micecam::domain {

class CameraStream {
public:
    virtual ~CameraStream() = default;

    virtual bool read_frame(std::vector<uint8_t>& out_data, int64_t& out_pts) = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int fps() const = 0;
    virtual std::string pixel_format() const = 0;
    virtual bool is_open() const = 0;
    virtual void close() = 0;
};

} // namespace micecam::domain

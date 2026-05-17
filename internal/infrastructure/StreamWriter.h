#pragma once

#include <mutex>
#include <string>

#include "pipeline/IStreamWriter.h"

struct AVFormatContext;
struct AVStream;

namespace micecam::infrastructure {

class StreamWriter : public pipeline::IStreamWriter {
public:
    StreamWriter();
    ~StreamWriter() override;

    StreamWriter(const StreamWriter&) = delete;
    StreamWriter& operator=(const StreamWriter&) = delete;

    bool open(const std::string& path, int width, int height, int fps) override;
    bool write_packet(const uint8_t* data, size_t size, int64_t pts, int64_t dts, bool keyframe) override;
    bool close() override;

private:
    std::mutex mutex_;
    AVFormatContext* fmt_ctx_ = nullptr;
    AVStream* stream_ = nullptr;
    int64_t packet_index_ = 0;
};

} // namespace micecam::infrastructure

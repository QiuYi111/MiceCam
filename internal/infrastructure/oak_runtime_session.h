#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace micecam {

struct OAKSessionConfig {
    int width = 1280;
    int height = 800;
    double fps = 30.0;
};

struct OAKEncodedFrame {
    uint64_t sequence_id = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    std::shared_ptr<std::vector<uint8_t>> data;
};

struct OAKFrameGroup {
    std::array<std::shared_ptr<OAKEncodedFrame>, 4> frames;
};

class OAKRuntimeSession {
public:
    OAKRuntimeSession();
    ~OAKRuntimeSession();

    bool initialize(const OAKSessionConfig& config);
    void stop();
    std::shared_ptr<OAKFrameGroup> get_group();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace micecam

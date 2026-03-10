#pragma once

#include "micecam/types.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace micecam {

struct Frame {
    uint64_t sequence_id;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::unique_ptr<std::vector<uint8_t>> data;
    PixelFormat format = PixelFormat::MJPEG;
    uint32_t width = 0;
    uint32_t height = 0;

    Frame() : sequence_id(0), timestamp(std::chrono::high_resolution_clock::now()) {}

    Frame(uint64_t seq, std::unique_ptr<std::vector<uint8_t>> frame_data)
        : sequence_id(seq),
          timestamp(std::chrono::high_resolution_clock::now()),
          data(std::move(frame_data)) {}

    // Non-copyable (ownership transfer)
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    // Movable
    Frame(Frame&&) = default;
    Frame& operator=(Frame&&) = default;

    [[nodiscard]] std::unique_ptr<Frame> clone() const {
        auto new_frame = std::make_unique<Frame>();
        new_frame->sequence_id = sequence_id;
        new_frame->timestamp = timestamp;
        new_frame->format = format;
        new_frame->width = width;
        new_frame->height = height;
        if (data) {
            new_frame->data = std::make_unique<std::vector<uint8_t>>(*data);
        }
        return new_frame;
    }

    [[nodiscard]] size_t size() const noexcept {
        return data ? data->size() : 0;
    }
};

}  // namespace micecam

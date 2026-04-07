#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace micecam {

struct Frame {
    uint64_t sequence_id;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::unique_ptr<std::vector<uint8_t>> data;

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

    [[nodiscard]] size_t size() const noexcept {
        return data ? data->size() : 0;
    }
};

}  // namespace micecam

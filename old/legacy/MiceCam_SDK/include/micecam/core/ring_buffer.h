#pragma once

#include "micecam/core/frame.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace micecam {

class RingBuffer {
public:
    explicit RingBuffer(size_t capacity);
    ~RingBuffer() = default;

    // Non-copyable
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // Producer API - blocks if full
    void push(Frame&& frame);

    // Producer API - non-blocking, returns false if full
    [[nodiscard]] bool try_push(Frame&& frame);

    // Consumer API - blocks if empty
    Frame pop();

    // Consumer API - non-blocking, returns nullptr if empty
    [[nodiscard]] std::unique_ptr<Frame> try_pop();

    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t size() const noexcept { return size_.load(); }
    [[nodiscard]] bool empty() const noexcept { return size_.load() == 0; }
    [[nodiscard]] bool full() const noexcept { return size_.load() == capacity_; }

private:
    const size_t capacity_;
    std::vector<Frame> buffer_;
    std::atomic<size_t> size_{0};
    size_t read_idx_ = 0;
    size_t write_idx_ = 0;

    mutable std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
};

}  // namespace micecam

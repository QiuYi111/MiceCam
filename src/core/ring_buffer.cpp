#include "micecam/core/ring_buffer.h"
#include <cassert>

namespace micecam {

RingBuffer::RingBuffer(size_t capacity)
    : capacity_(capacity), buffer_(capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("Ring buffer capacity must be > 0");
    }
}

void RingBuffer::push(Frame&& frame) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_not_full_.wait(lock, [this] { return size_.load() < capacity_; });

    buffer_[write_idx_] = std::move(frame);
    write_idx_ = (write_idx_ + 1) % capacity_;
    size_.fetch_add(1);
    cv_not_empty_.notify_one();
}

bool RingBuffer::try_push(Frame&& frame) {
    std::unique_lock<std::mutex> lock(mtx_);
    if (size_.load() >= capacity_) {
        return false;
    }

    buffer_[write_idx_] = std::move(frame);
    write_idx_ = (write_idx_ + 1) % capacity_;
    size_.fetch_add(1);
    cv_not_empty_.notify_one();
    return true;
}

Frame RingBuffer::pop() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_not_empty_.wait(lock, [this] { return size_.load() > 0; });

    Frame frame = std::move(buffer_[read_idx_]);
    read_idx_ = (read_idx_ + 1) % capacity_;
    size_.fetch_sub(1);
    cv_not_full_.notify_one();
    return frame;
}

std::unique_ptr<Frame> RingBuffer::try_pop() {
    std::unique_lock<std::mutex> lock(mtx_);
    if (size_.load() == 0) {
        return nullptr;
    }

    auto frame = std::make_unique<Frame>(std::move(buffer_[read_idx_]));
    read_idx_ = (read_idx_ + 1) % capacity_;
    size_.fetch_sub(1);
    cv_not_full_.notify_one();
    return frame;
}

}  // namespace micecam

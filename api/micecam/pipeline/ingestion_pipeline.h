#pragma once

#include "micecam/camera/camera_backend.h"
#include "domain/ring_buffer.h"
#include "micecam/pipeline/disk_writer.h"
#include "micecam/pipeline/frame_dispatcher.h"
#include "micecam/types.h"
#include <atomic>
#include <memory>
#include <thread>

namespace micecam {

class IngestionPipeline {
public:
    IngestionPipeline(std::unique_ptr<ICameraBackend> camera,
                     const SessionConfig& session_config);
    ~IngestionPipeline();

    // Non-copyable
    IngestionPipeline(const IngestionPipeline&) = delete;
    IngestionPipeline& operator=(const IngestionPipeline&) = delete;

    // Start/stop the pipeline
    bool start();
    void stop();

    // Wait for completion
    void join();

    // Observer management (thread-safe)
    void attach_observer(std::shared_ptr<IFrameObserver> observer);
    void detach_observer(std::shared_ptr<IFrameObserver> observer);

    // Get statistics
    [[nodiscard]] uint64_t get_frames_captured() const {
        return frames_captured_.load();
    }

    [[nodiscard]] uint64_t get_frames_dropped() const {
        return frames_dropped_.load();
    }

    [[nodiscard]] double get_drop_rate() const {
        const uint64_t total = frames_captured_.load() + frames_dropped_.load();
        if (total == 0) return 0.0;
        return static_cast<double>(frames_dropped_.load()) / total;
    }

    [[nodiscard]] bool is_running() const {
        return running_.load();
    }

    [[nodiscard]] const DiskWriter& get_writer() const {
        return writer_;
    }

    // RFC-001: Get pipeline stats
    [[nodiscard]] PipelineStats get_stats() const;

private:
    void camera_thread_func();

    std::unique_ptr<ICameraBackend> camera_;
    RingBuffer buffer_;
    DiskWriter writer_;
    FrameDispatcher dispatcher_;

    std::atomic<bool> running_{false};
    std::atomic<uint64_t> frames_captured_{0};
    std::atomic<uint64_t> frames_dropped_{0};

    std::thread camera_thread_;
    std::thread writer_thread_;

    // For stats calculation
    SessionConfig config_;
};

}  // namespace micecam

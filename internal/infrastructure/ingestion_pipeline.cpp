#include "infrastructure/ingestion_pipeline.h"
#include <iostream>

namespace micecam {

IngestionPipeline::IngestionPipeline(
    std::unique_ptr<ICameraBackend> camera,
    const SessionConfig& session_config)
    : camera_(std::move(camera)),
      buffer_(session_config.ring_buffer_size),
      writer_(session_config),
      config_(session_config) {
}

IngestionPipeline::~IngestionPipeline() {
    stop();
}

bool IngestionPipeline::start() {
    if (running_.load()) {
        return false;
    }

    // Start camera
    if (!camera_->start()) {
        std::cerr << "Failed to start camera backend\n";
        return false;
    }

    // Start disk writer
    if (!writer_.start()) {
        std::cerr << "Failed to start disk writer\n";
        camera_->stop();
        return false;
    }

    // Start writer thread (consumer from ring buffer)
    writer_.consume_from(buffer_);

    running_.store(true);
    frames_captured_.store(0);

    // Start camera thread (producer to ring buffer)
    camera_thread_ = std::thread(&IngestionPipeline::camera_thread_func, this);

    std::cout << "Ingestion pipeline started\n";
    return true;
}

void IngestionPipeline::stop() {
    if (!running_.load()) {
        return;
    }

    std::cout << "Stopping ingestion pipeline...\n";

    running_.store(false);

    // Stop camera FIRST to unblock any pending get_frame() calls
    camera_->stop();

    // Wait for camera thread
    if (camera_thread_.joinable()) {
        camera_thread_.join();
    }

    // Stop writer (this will finalize the session)
    writer_.stop();

    std::cout << "Pipeline stopped. Frames captured: " << frames_captured_.load() << "\n";
}

void IngestionPipeline::join() {
    if (camera_thread_.joinable()) {
        camera_thread_.join();
    }
    // Writer thread is managed by DiskWriter
}

void IngestionPipeline::attach_observer(std::shared_ptr<IFrameObserver> observer) {
    dispatcher_.attach(observer);
}

void IngestionPipeline::detach_observer(std::shared_ptr<IFrameObserver> observer) {
    dispatcher_.detach(observer);
}

PipelineStats IngestionPipeline::get_stats() const {
    PipelineStats stats;
    stats.captured_frames = frames_captured_.load();
    stats.dropped_frames = frames_dropped_.load();
    stats.drop_rate = get_drop_rate();
    stats.pending_buffer_size = buffer_.size();  // size() is already a function
    // Throughput calculation would require timing; simplified here
    stats.current_throughput_mbps = 0.0;
    return stats;
}

void IngestionPipeline::camera_thread_func() {
    std::cout << "Camera thread started\n";

    uint64_t sequence_id = 0;

    while (running_.load()) {
        auto frame = camera_->get_frame();

        if (!frame) {
            // Camera stopped or error
            if (running_.load()) {
                std::cerr << "Camera returned null frame while running\n";
            }
            break;
        }

        // RFC-001: Dispatch to observers (best-effort, non-blocking)
        // Create a FrameView for observers before moving the frame
        FrameView view;
        view.data = frame->data ? frame->data->data() : nullptr;
        view.size = frame->size();
        view.sequence_id = frame->sequence_id;
        view.timestamp = std::chrono::duration<double>(frame->timestamp.time_since_epoch()).count();
        view.format = PixelFormat::MJPEG;  // Assuming MJPEG; could be dynamic
        view.width = config_.width;
        view.height = config_.height;
        view.metadata_json = nullptr;

        dispatcher_.dispatch(view);

        // Critical path: push to ring buffer for disk write
        if (!buffer_.try_push(std::move(*frame))) {
            frames_dropped_.fetch_add(1);
        } else {
            frames_captured_.fetch_add(1);
        }
    }

    std::cout << "Camera thread stopped. "
              << "Captured: " << frames_captured_.load()
              << ", Dropped: " << frames_dropped_.load()
              << " (" << (get_drop_rate() * 100) << "%)\n";
}

}  // namespace micecam

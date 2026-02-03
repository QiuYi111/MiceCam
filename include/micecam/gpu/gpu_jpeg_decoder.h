#pragma once

#include "micecam/observer.h"
#include <memory>
#include <cstdint>

// Forward declare OpenGL types to avoid header pollution
using GLuint = unsigned int;

namespace micecam {

/**
 * @brief GPU-accelerated MJPEG decoder using NVIDIA NVDEC.
 * 
 * This class implements IFrameObserver to receive MJPEG frames from the
 * pipeline and decode them on the GPU using NVIDIA's hardware decoder.
 * The decoded frames are made available as OpenGL textures for rendering.
 * 
 * **Design Philosophy:**
 * - Non-blocking: If the GPU queue is full, frames are dropped rather than
 *   blocking the pipeline's critical recording path.
 * - Zero-copy: Uses CUDA-OpenGL interop to avoid copying decoded frames
 *   back to CPU memory.
 * 
 * **Requirements:**
 * - NVIDIA GPU with NVDEC support (Kepler or newer)
 * - CUDA Toolkit 11.x+
 * - NVIDIA Video Codec SDK
 * 
 * @note This class is NOT thread-safe for rendering. All get_texture() calls
 * must be made from the same thread that created the OpenGL context.
 */
class GpuJpegDecoder : public IFrameObserver {
public:
    /**
     * @brief Construct a GPU decoder for the specified frame dimensions.
     * 
     * @param width Frame width in pixels
     * @param height Frame height in pixels
     * @param queue_depth Number of frames to buffer on GPU (default: 3)
     * @throws std::runtime_error if CUDA/NVDEC initialization fails
     */
    GpuJpegDecoder(int width, int height, int queue_depth = 3);
    
    ~GpuJpegDecoder() override;
    
    // Non-copyable, non-movable (owns GPU resources)
    GpuJpegDecoder(const GpuJpegDecoder&) = delete;
    GpuJpegDecoder& operator=(const GpuJpegDecoder&) = delete;
    GpuJpegDecoder(GpuJpegDecoder&&) = delete;
    GpuJpegDecoder& operator=(GpuJpegDecoder&&) = delete;
    
    /**
     * @brief Receive a frame from the pipeline and submit for GPU decoding.
     * 
     * This method is called by the FrameDispatcher on the camera thread.
     * It performs an asynchronous memcpy to GPU and submits the decode job.
     * 
     * **Performance Note:** This method returns quickly (<1ms). The actual
     * decoding happens asynchronously on the GPU.
     * 
     * @param frame The MJPEG frame to decode
     */
    void on_frame(const FrameView& frame) override;
    
    /**
     * @brief Get the OpenGL texture ID for the most recently decoded frame.
     * 
     * This texture is suitable for rendering with ImGui, Qt, or raw OpenGL.
     * The texture format is RGBA8.
     * 
     * @return OpenGL texture ID, or 0 if no frame is available
     */
    GLuint get_texture() const;
    
    /**
     * @brief Get the sequence ID of the currently displayed frame.
     */
    uint64_t get_current_frame_id() const;
    
    /**
     * @brief Get the number of frames dropped due to GPU queue overflow.
     */
    uint64_t get_dropped_frames() const;
    
    /**
     * @brief Check if the decoder is healthy (GPU accessible, no errors).
     */
    bool is_healthy() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace micecam

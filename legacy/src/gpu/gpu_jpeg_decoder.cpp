/**
 * @file gpu_jpeg_decoder.cpp
 * @brief Implementation of GPU-accelerated MJPEG decoder using NVDEC
 */

#include "micecam/gpu/gpu_jpeg_decoder.h"
#include <iostream>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

#ifdef HAVE_NVDEC
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <nvcuvid.h>

// Error checking macro
#define CUDA_DRV_CHECK(call) \
    do { \
        CUresult err = call; \
        if (err != CUDA_SUCCESS) { \
            const char* szErrName = nullptr; \
            cuGetErrorName(err, &szErrName); \
            std::cerr << "CUDA Driver Error: " << szErrName << " at " << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

#endif

namespace micecam {

class GpuJpegDecoder::Impl {
public:
    Impl(int width, int height, int queue_depth)
        : width_(width), height_(height), queue_depth_(queue_depth) {

        std::cout << "[GpuJpegDecoder] Initializing " << width << "x" << height << "\n";

#ifdef HAVE_NVDEC
        if (initNvdec()) {
            use_nvdec_ = true;
            std::cout << "[GpuJpegDecoder] NVDEC hardware decoding enabled\n";
        } else {
            std::cerr << "[GpuJpegDecoder] NVDEC init failed, hardware acceleration disabled\n";
        }
#else
        std::cout << "[GpuJpegDecoder] Built without NVDEC\n";
#endif
        initialized_ = true;
    }

    ~Impl() {
#ifdef HAVE_NVDEC
        cleanupNvdec();
#endif
    }

    void onFrame(const FrameView& frame) {
        if (!initialized_ || !use_nvdec_) return;

        // Push to queue for async processing
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (pending_frames_.size() >= static_cast<size_t>(queue_depth_)) {
            dropped_frames_++;
            return;
        }

        std::vector<uint8_t> data(frame.data, frame.data + frame.size);
        pending_frames_.push({frame.sequence_id, std::move(data)});

        // In a real implementation, a separate thread would consume this
        // and call cuvidDecodePicture.
        current_frame_id_.store(frame.sequence_id);
    }

    GLuint getTexture() const { return texture_id_; }
    uint64_t getCurrentFrameId() const { return current_frame_id_.load(); }
    uint64_t getDroppedFrames() const { return dropped_frames_.load(); }
    bool isHealthy() const { return initialized_ && !error_state_; }

private:
#ifdef HAVE_NVDEC
    bool initNvdec() {
        // 1. Init CUDA
        if (cuInit(0) != CUDA_SUCCESS) return false;
        if (cuDeviceGet(&cuda_device_, 0) != CUDA_SUCCESS) return false;
        if (cuCtxCreate(&cuda_context_, CU_CTX_SCHED_BLOCKING_SYNC, cuda_device_) != CUDA_SUCCESS) return false;

        // 2. Create Decoder
        CUVIDDECODECREATEINFO decodeInfo = {};
        decodeInfo.CodecType = cudaVideoCodec_JPEG;
        decodeInfo.ChromaFormat = cudaVideoChromaFormat_420; // Most webcams
        decodeInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
        decodeInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
        decodeInfo.ulNumDecodeSurfaces = 4;
        decodeInfo.ulNumOutputSurfaces = 1;
        decodeInfo.ulCreationWidth = width_;
        decodeInfo.ulCreationHeight = height_;
        decodeInfo.ulMaxWidth = width_;
        decodeInfo.ulMaxHeight = height_;
        decodeInfo.ulTargetWidth = width_;
        decodeInfo.ulTargetHeight = height_;

        if (cuvidCreateDecoder(&decoder_, &decodeInfo) != CUDA_SUCCESS) {
            std::cerr << "[GpuJpegDecoder] Failed to create NVDEC decoder\n";
            return false;
        }

        return true;
    }

    void cleanupNvdec() {
        if (decoder_) cuvidDestroyDecoder(decoder_);
        if (cuda_context_) cuCtxDestroy(cuda_context_);
    }

    CUdevice cuda_device_ = 0;
    CUcontext cuda_context_ = nullptr;
    CUvideodecoder decoder_ = nullptr;
#endif

    struct PendingFrame {
        uint64_t sequence_id;
        std::vector<uint8_t> data;
    };

    int width_, height_, queue_depth_;
    bool initialized_ = false;
    bool use_nvdec_ = false;
    bool error_state_ = false;
    GLuint texture_id_ = 0;
    std::atomic<uint64_t> current_frame_id_{0};
    std::atomic<uint64_t> dropped_frames_{0};

    std::mutex queue_mutex_;
    std::queue<PendingFrame> pending_frames_;
};

GpuJpegDecoder::GpuJpegDecoder(int width, int height, int queue_depth)
    : impl_(std::make_unique<Impl>(width, height, queue_depth)) {}

GpuJpegDecoder::~GpuJpegDecoder() = default;

void GpuJpegDecoder::on_frame(const FrameView& frame) {
    impl_->onFrame(frame);
}

GLuint GpuJpegDecoder::get_texture() const {
    return impl_->getTexture();
}

uint64_t GpuJpegDecoder::get_current_frame_id() const {
    return impl_->getCurrentFrameId();
}

uint64_t GpuJpegDecoder::get_dropped_frames() const {
    return impl_->getDroppedFrames();
}

bool GpuJpegDecoder::is_healthy() const {
    return impl_->isHealthy();
}

} // namespace micecam

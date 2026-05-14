#pragma once

#include <cstdint>
#include <string>
#include <map>

namespace micecam {

/**
 * @brief Pixel format enumeration for frame data.
 */
enum class PixelFormat {
    MJPEG,      ///< Compressed MJPEG stream (Webcam Default)
    RGB24,      ///< Raw RGB 24-bit
    MONO8,      ///< 8-bit grayscale
    MONO16,     ///< 16-bit grayscale (FLIR/Scientific cameras)
    NV12,       ///< YUV 4:2:0 (GPU Decode Output)
    UYVY422     ///< YUV 4:2:2 Packed (Mac Camera Default)
};

inline std::string PixelFormatToString(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::MJPEG:   return "mjpeg";
        case PixelFormat::RGB24:   return "rgb24";
        case PixelFormat::MONO8:   return "mono8";
        case PixelFormat::MONO16:  return "mono16";
        case PixelFormat::NV12:    return "nv12";
        case PixelFormat::UYVY422: return "uyvy422";
        default: return "unknown";
    }
}

/**
 * @brief Read-only view of a frame for observer callbacks.
 *
 * This is a lightweight, non-owning view. The data pointer is only valid
 * during the callback invocation. Observers that need to retain data
 * must copy it.
 */
struct FrameView {
    const uint8_t* data;       ///< Pointer to frame data (e.g., MJPEG bitstream)
    size_t size;               ///< Data size in bytes
    uint64_t sequence_id;      ///< Global unique frame sequence number
    double timestamp;          ///< Hardware timestamp in seconds
    PixelFormat format;        ///< Pixel format
    uint32_t width;            ///< Frame width
    uint32_t height;           ///< Frame height
    const char* metadata_json; ///< Optional JSON metadata (e.g., gain, temperature)
};

/**
 * @brief System configuration for pipeline initialization.
 */
struct SystemConfig {
    // Hardware selection
    std::string backend_name = "ffmpeg";  ///< Backend: "ffmpeg", "oak", "flir"
    int device_id = 0;                    ///< Device index

    // Capture parameters
    int width = 1920;
    int height = 1080;
    double fps = 30.0;

    // Storage parameters
    std::string session_name;
    std::string output_dir;
    size_t ring_buffer_size = 200;        ///< Buffer size for jitter handling
    bool enable_disk_write = true;        ///< Set false for preview-only mode
    bool enable_checksums = true;         ///< Enable CRC32 integrity checks

    // Backend-specific options (flexible extension)
    std::map<std::string, std::string> backend_options;
};

/**
 * @brief Runtime statistics for the pipeline.
 */
struct PipelineStats {
    uint64_t captured_frames = 0;
    uint64_t dropped_frames = 0;
    double drop_rate = 0.0;
    double current_throughput_mbps = 0.0;
    uint64_t pending_buffer_size = 0;
};

} // namespace micecam

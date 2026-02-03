#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace micecam {

struct HDF5ConversionConfig {
    std::string bin_file_path;
    std::string metadata_file_path;
    std::string output_hdf5_path;

    // Conversion options
    bool compress_data = false;  // HDF5 compression (optional)
    int compression_level = 6;   // 0-9, default 6
};

class HDF5Converter {
public:
    explicit HDF5Converter(const HDF5ConversionConfig& config);
    ~HDF5Converter();

    // Non-copyable
    HDF5Converter(const HDF5Converter&) = delete;
    HDF5Converter& operator=(const HDF5Converter&) = delete;

    // Execute conversion
    bool convert();

    // Verification
    [[nodiscard]] static bool verify_hdf5_file(const std::string& hdf5_path);

private:
    HDF5ConversionConfig config_;

    // Internal helpers
    [[nodiscard]] bool read_metadata();
    [[nodiscard]] bool write_hdf5();

    // Metadata storage (parsed from JSON)
    struct FrameInfo {
        uint64_t sequence_id;
        uint64_t timestamp_ns;
        uint64_t offset;
        uint64_t size;
        uint32_t checksum;
    };

    std::vector<FrameInfo> frames_;
    uint64_t total_frames_ = 0;
    uint64_t total_bytes_ = 0;
    uint64_t start_timestamp_ns_ = 0;
    uint64_t end_timestamp_ns_ = 0;
};

}  // namespace micecam

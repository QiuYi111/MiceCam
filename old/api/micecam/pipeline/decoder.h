#pragma once

#include <string>
#include <functional>

namespace micecam {

struct DecoderConfig {
    std::string bin_path;
    std::string jsonl_path;
    std::string target_dir;
};

class Decoder {
public:
    using ProgressCallback = std::function<void(float progress)>;

    Decoder() = default;

    /**
     * @brief Decodes a single .bin/.jsonl pair into JPEGs.
     * @return True if successful, false otherwise.
     */
    bool decode_session(const DecoderConfig& config, ProgressCallback progress_cb = nullptr);

    /**
     * @brief High-level helper for multi-camera sessions (CAM_A, CAM_B, etc.)
     */
    bool decode_micecam_project(const std::string& output_dir,
                               const std::string& session_name,
                               const std::string& target_parent_dir,
                               ProgressCallback progress_cb = nullptr);

private:
    bool decode_single_file(const std::string& bin_path,
                           const std::string& jsonl_path,
                           const std::string& target_dir,
                           ProgressCallback progress_cb);
};

} // namespace micecam

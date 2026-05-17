#include "micecam/pipeline/decoder.h"
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <chrono>

namespace micecam {

namespace fs = std::filesystem;
using json = nlohmann::json;

bool Decoder::decode_session(const DecoderConfig& config, ProgressCallback progress_cb) {
    return decode_single_file(config.bin_path, config.jsonl_path, config.target_dir, progress_cb);
}

bool Decoder::decode_micecam_project(const std::string& output_dir,
                                   const std::string& session_name,
                                   const std::string& target_parent_dir,
                                   ProgressCallback progress_cb) {
    std::vector<std::string> suffixes = {"", "_A", "_B", "_C", "_D"};
    struct Sensor {
        std::string bin;
        std::string jsonl;
        std::string label;
    };
    std::vector<Sensor> sensors;

    for (const auto& s : suffixes) {
        std::string bin = (fs::path(output_dir) / (session_name + s + ".bin")).string();
        std::string jsonl = (fs::path(output_dir) / (session_name + s + "_metadata.jsonl")).string();
        if (fs::exists(bin) && fs::exists(jsonl)) {
            sensors.push_back({bin, jsonl, s.empty() ? "" : "CAM" + s});
        }
    }

    if (sensors.empty()) return false;

    fs::create_directories(target_parent_dir);

    for (size_t i = 0; i < sensors.size(); ++i) {
        fs::path target = fs::path(target_parent_dir);
        if (!sensors[i].label.empty()) target /= sensors[i].label;
        fs::create_directories(target);

        float base_progress = (float)i / sensors.size() * 100.0f;
        auto sub_cb = [&](float p) {
            if (progress_cb) progress_cb(base_progress + (p / sensors.size()));
        };

        if (!decode_single_file(sensors[i].bin, sensors[i].jsonl, target.string(), sub_cb)) {
            return false;
        }
    }

    if (progress_cb) progress_cb(100.0f);
    return true;
}

bool Decoder::decode_single_file(const std::string& bin_path,
                               const std::string& jsonl_path,
                               const std::string& target_dir,
                               ProgressCallback progress_cb) {
    std::ifstream f_bin(bin_path, std::ios::binary);
    std::ifstream f_jsonl(jsonl_path);
    if (!f_bin || !f_jsonl) return false;

    auto total_bytes = fs::file_size(bin_path);
    if (total_bytes == 0) return true;

    // 1. Detect format from session_start
    std::string pixel_format_str = "mjpeg";
    uint32_t width = 1920, height = 1080;

    std::string line;
    while (std::getline(f_jsonl, line)) {
        try {
            auto msg = json::parse(line);
            if (msg.value("type", "") == "session_start") {
                pixel_format_str = msg.value("pixel_format", "mjpeg");
                width = msg.value("width", width);
                height = msg.value("height", height);
                break;
            }
        } catch (...) {}
    }

    // Reset jsonl to start
    f_jsonl.clear();
    f_jsonl.seekg(0);

    int frame_count = 0;
    while (std::getline(f_jsonl, line)) {
        try {
            auto msg = json::parse(line);
            if (msg.value("type", "") != "frame") continue;

            uint64_t offset = msg.at("offset");
            uint32_t size = msg.at("size");
            uint64_t ts_ns = msg.at("timestamp_ns");

            f_bin.seekg(offset);
            std::vector<uint8_t> buffer(size);
            f_bin.read(reinterpret_cast<char*>(buffer.data()), size);

            fs::path img_path = fs::path(target_dir) / (std::to_string(ts_ns) + ".jpg");

            if (pixel_format_str == "uyvy422") {
                cv::Mat yuv(height, width, CV_8UC2, buffer.data());
                cv::Mat bgr;
                cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_UYVY);
                cv::imwrite(img_path.string(), bgr, {cv::IMWRITE_JPEG_QUALITY, 95});
            } else {
                // Assume MJPEG
                std::ofstream f_img(img_path, std::ios::binary);
                f_img.write(reinterpret_cast<char*>(buffer.data()), size);
            }

            frame_count++;
            if (frame_count % 20 == 0 && progress_cb) {
                progress_cb((float)offset / total_bytes * 100.0f);
            }
        } catch (...) {
            continue;
        }
    }

    return true;
}

} // namespace micecam

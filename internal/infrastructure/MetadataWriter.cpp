#include "infrastructure/MetadataWriter.h"

#include <spdlog/spdlog.h>

#include <cinttypes>
#include <chrono>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <nlohmann/json.hpp>

namespace micecam::infrastructure {

static std::string ns_to_iso8601(uint64_t ns) {
    if (ns == 0) return "";
    auto total_sec = static_cast<time_t>(ns / 1000000000ULL);
    uint64_t remaining_ns = ns % 1000000000ULL;
    uint64_t microseconds = remaining_ns / 1000ULL;

    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &total_sec);
#else
    localtime_r(&total_sec, &tm_buf);
#endif

    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);

    char result[96];
    snprintf(result, sizeof(result), "%s.%06" PRIu64, time_buf, microseconds);
    return std::string(result);
}

bool MetadataWriter::write_session_header(const domain::SessionMetadata& meta, const std::string& path) {
    try {
        nlohmann::json j = meta.to_json();

        std::string wall_time_str;
        if (meta.start_time_ns != 0) {
            wall_time_str = ns_to_iso8601(meta.start_time_ns);
        } else {
            auto now = std::chrono::system_clock::now();
            auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()).count();
            wall_time_str = ns_to_iso8601(static_cast<uint64_t>(now_ns));
        }
        if (!wall_time_str.empty()) {
            j["session_start_wall_time"] = wall_time_str;
        }

        std::ofstream out(path);
        if (!out.is_open()) {
            spdlog::error("Failed to open metadata file: {}", path);
            return false;
        }
        out << j.dump(2);
        out.close();
        spdlog::info("MetadataWriter wrote session header: {}", path);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to write session header: {}", e.what());
        return false;
    }
}

bool MetadataWriter::write_session_footer(const std::string& path, uint64_t total_frames, uint64_t total_bytes, const std::string& session_checksum) {
    try {
        std::ifstream in(path);
        if (!in.is_open()) {
            spdlog::error("Failed to open metadata file for footer update: {}", path);
            return false;
        }
        nlohmann::json j;
        in >> j;
        in.close();

        j["total_frames"] = total_frames;
        j["total_bytes"] = total_bytes;
        j["session_checksum"] = session_checksum;

        std::ofstream out(path);
        if (!out.is_open()) {
            spdlog::error("Failed to open metadata file for footer write: {}", path);
            return false;
        }
        out << j.dump(2);
        out.close();
        spdlog::info("MetadataWriter wrote session footer: {}", path);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to write session footer: {}", e.what());
        return false;
    }
}

bool MetadataWriter::write_stats(const std::string& path, const std::vector<domain::StreamStats>& stats) {
    try {
        nlohmann::json obj = nlohmann::json::object();
        for (const auto& s : stats) {
            obj[s.stream_id] = s.to_json();
        }
        std::ofstream out(path);
        if (!out.is_open()) {
            spdlog::error("Failed to open stats file: {}", path);
            return false;
        }
        out << obj.dump(2);
        out.close();
        spdlog::info("MetadataWriter wrote {} stream stats: {}", stats.size(), path);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to write stats: {}", e.what());
        return false;
    }
}

} // namespace micecam::infrastructure

#include "infrastructure/MetadataWriter.h"

#include <spdlog/spdlog.h>

#include <fstream>

#include <nlohmann/json.hpp>

namespace micecam::infrastructure {

bool MetadataWriter::write_session_header(const domain::SessionMetadata& meta, const std::string& path) {
    try {
        nlohmann::json j = meta.to_json();
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
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& s : stats) {
            arr.push_back(s.to_json());
        }
        std::ofstream out(path);
        if (!out.is_open()) {
            spdlog::error("Failed to open stats file: {}", path);
            return false;
        }
        out << arr.dump(2);
        out.close();
        spdlog::info("MetadataWriter wrote {} stream stats: {}", stats.size(), path);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to write stats: {}", e.what());
        return false;
    }
}

} // namespace micecam::infrastructure

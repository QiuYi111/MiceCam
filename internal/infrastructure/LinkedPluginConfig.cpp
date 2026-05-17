#include "LinkedPluginConfig.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <nlohmann/json.hpp>

namespace micecam::infrastructure {

static std::string iso8601_now() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

LinkedPluginConfig::LinkedPluginConfig(const std::string& config_path)
    : config_path_(config_path) {}

bool LinkedPluginConfig::load() {
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        return true;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }

    if (j.contains("linked_plugins") && j["linked_plugins"].is_array()) {
        entries_.clear();
        for (const auto& item : j["linked_plugins"]) {
            LinkedPluginEntry entry;
            if (item.is_string()) {
                entry.path = item.get<std::string>();
                entry.enabled = true;
                entry.added_at = "";
            } else if (item.is_object()) {
                if (item.contains("path") && item["path"].is_string()) {
                    entry.path = item["path"].get<std::string>();
                }
                if (item.contains("enabled") && item["enabled"].is_boolean()) {
                    entry.enabled = item["enabled"].get<bool>();
                } else {
                    entry.enabled = true;
                }
                if (item.contains("added_at") && item["added_at"].is_string()) {
                    entry.added_at = item["added_at"].get<std::string>();
                }
            } else {
                continue;
            }
            if (!entry.path.empty()) {
                entries_.push_back(std::move(entry));
            }
        }
    }
    return true;
}

bool LinkedPluginConfig::save() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& e : entries_) {
        nlohmann::json obj;
        obj["path"] = e.path;
        obj["enabled"] = e.enabled;
        obj["added_at"] = e.added_at;
        arr.push_back(obj);
    }

    nlohmann::json j;
    j["linked_plugins"] = arr;

    std::ofstream file(config_path_);
    if (!file.is_open()) return false;
    file << j.dump(2);
    return file.good();
}

void LinkedPluginConfig::add(const std::string& path) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const LinkedPluginEntry& e) { return e.path == path; });
    if (it == entries_.end()) {
        entries_.push_back({path, true, iso8601_now()});
    }
}

void LinkedPluginConfig::remove(const std::string& path) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const LinkedPluginEntry& e) { return e.path == path; });
    if (it != entries_.end()) {
        entries_.erase(it);
    }
}

std::vector<std::string> LinkedPluginConfig::paths() const {
    std::vector<std::string> result;
    for (const auto& e : entries_) {
        if (e.enabled) {
            result.push_back(e.path);
        }
    }
    return result;
}

const std::vector<LinkedPluginEntry>& LinkedPluginConfig::entries() const {
    return entries_;
}

bool LinkedPluginConfig::set_enabled(const std::string& path, bool enabled) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const LinkedPluginEntry& e) { return e.path == path; });
    if (it == entries_.end()) return false;
    it->enabled = enabled;
    return true;
}

} // namespace micecam::infrastructure

#include "LinkedPluginConfig.h"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

namespace micecam::infrastructure {

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
        paths_.clear();
        for (const auto& entry : j["linked_plugins"]) {
            if (entry.is_string()) {
                paths_.push_back(entry.get<std::string>());
            }
        }
    }
    return true;
}

bool LinkedPluginConfig::save() const {
    nlohmann::json j;
    j["linked_plugins"] = paths_;

    std::ofstream file(config_path_);
    if (!file.is_open()) return false;
    file << j.dump(2);
    return file.good();
}

void LinkedPluginConfig::add(const std::string& path) {
    auto it = std::find(paths_.begin(), paths_.end(), path);
    if (it == paths_.end()) {
        paths_.push_back(path);
    }
}

void LinkedPluginConfig::remove(const std::string& path) {
    auto it = std::find(paths_.begin(), paths_.end(), path);
    if (it != paths_.end()) {
        paths_.erase(it);
    }
}

std::vector<std::string> LinkedPluginConfig::paths() const {
    return paths_;
}

} // namespace micecam::infrastructure

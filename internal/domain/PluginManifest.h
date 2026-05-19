#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace micecam::domain {

struct PlatformEntrypoint {
    std::string entrypoint;
    std::string arch;
};

struct PluginManifest {
    std::string id;
    std::string name;
    std::string version;
    uint32_t plugin_api_version = 1;
    std::string min_micecam_version;
    std::string description;
    std::string author;
    std::unordered_map<std::string, PlatformEntrypoint> platforms;
    std::vector<std::string> required_features;
    std::vector<std::string> optional_features;
    std::vector<std::string> supported_process_models;
    std::string preferred_process_model;

    static PluginManifest from_json(const nlohmann::json& j);
    nlohmann::json to_json() const;
    std::vector<std::string> validate() const;
};

} // namespace micecam::domain

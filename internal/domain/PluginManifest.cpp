#include "PluginManifest.h"

#include <algorithm>
#include <regex>

namespace micecam::domain {

PluginManifest PluginManifest::from_json(const nlohmann::json& j) {
    PluginManifest m;
    m.id = j.value("id", "");
    m.name = j.value("name", "");
    m.version = j.value("version", "");
    m.plugin_api_version = j.value("plugin_api_version", 1u);
    m.min_micecam_version = j.value("min_micecam_version", "");
    m.description = j.value("description", "");
    m.author = j.value("author", "");

    if (j.contains("platforms")) {
        for (const auto& [platform, entry] : j["platforms"].items()) {
            PlatformEntrypoint pe;
            pe.entrypoint = entry.value("entrypoint", "");
            pe.arch = entry.value("arch", "");
            m.platforms[platform] = pe;
        }
    }

    if (j.contains("required_features")) {
        m.required_features = j["required_features"].get<std::vector<std::string>>();
    }
    if (j.contains("optional_features")) {
        m.optional_features = j["optional_features"].get<std::vector<std::string>>();
    }
    if (j.contains("supported_process_models")) {
        m.supported_process_models = j["supported_process_models"].get<std::vector<std::string>>();
    }
    m.preferred_process_model = j.value("preferred_process_model", "");

    return m;
}

nlohmann::json PluginManifest::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["name"] = name;
    j["version"] = version;
    j["plugin_api_version"] = plugin_api_version;
    j["min_micecam_version"] = min_micecam_version;
    if (!description.empty()) j["description"] = description;
    if (!author.empty()) j["author"] = author;

    auto plat = nlohmann::json::object();
    for (const auto& [name, pe] : platforms) {
        auto entry = nlohmann::json::object();
        entry["entrypoint"] = pe.entrypoint;
        if (!pe.arch.empty()) entry["arch"] = pe.arch;
        plat[name] = entry;
    }
    j["platforms"] = plat;

    j["required_features"] = required_features;
    j["optional_features"] = optional_features;
    j["supported_process_models"] = supported_process_models;
    j["preferred_process_model"] = preferred_process_model;
    return j;
}

static const std::regex SEMVER_RE(R"re(^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(-[a-zA-Z0-9._]+)?(\+[a-zA-Z0-9._]+)?)re");

static const std::vector<std::string> VALID_PROCESS_MODELS = {"SINGLETON", "PER_DEVICE", "PER_STREAM"};

std::vector<std::string> PluginManifest::validate() const {
    std::vector<std::string> errors;

    if (id.empty()) errors.push_back("id is required");
    if (name.empty()) errors.push_back("name is required");
    if (version.empty()) errors.push_back("version is required");
    else if (!std::regex_match(version, SEMVER_RE)) errors.push_back("version must be valid semver");

    if (plugin_api_version == 0) errors.push_back("plugin_api_version must be >= 1");
    if (min_micecam_version.empty()) errors.push_back("min_micecam_version is required");
    else if (!std::regex_match(min_micecam_version, SEMVER_RE)) errors.push_back("min_micecam_version must be valid semver");

    if (platforms.empty()) errors.push_back("at least one platform entrypoint is required");
    for (const auto& [name, pe] : platforms) {
        if (pe.entrypoint.empty()) errors.push_back("platform '" + name + "' has empty entrypoint");
    }

    if (supported_process_models.empty()) errors.push_back("supported_process_models must have at least one entry");
    else {
        for (const auto& m : supported_process_models) {
            if (std::find(VALID_PROCESS_MODELS.begin(), VALID_PROCESS_MODELS.end(), m) == VALID_PROCESS_MODELS.end()) {
                errors.push_back("invalid process model: " + m);
            }
        }
    }

    if (preferred_process_model.empty()) errors.push_back("preferred_process_model is required");
    else if (std::find(VALID_PROCESS_MODELS.begin(), VALID_PROCESS_MODELS.end(), preferred_process_model) == VALID_PROCESS_MODELS.end()) {
        errors.push_back("invalid preferred_process_model: " + preferred_process_model);
    }
    else if (std::find(supported_process_models.begin(), supported_process_models.end(), preferred_process_model) == supported_process_models.end()) {
        errors.push_back("preferred_process_model '" + preferred_process_model + "' is not in supported_process_models");
    }

    return errors;
}

} // namespace micecam::domain

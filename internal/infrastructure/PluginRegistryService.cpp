#include "PluginRegistryService.h"

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

namespace micecam::infrastructure {

namespace fs = std::filesystem;

PluginRegistryService::PluginRegistryService(const std::string& bundled_plugins_dir,
                                             const std::string& config_dir)
    : bundled_plugins_dir_(bundled_plugins_dir)
    , linked_config_(config_dir + "/linked_plugins.json") {}

bool PluginRegistryService::initialize() {
    linked_config_.load();

    scanBundledDirectory();
    scanLinkedDirectories();

    pending_restart_ = false;
    initialized_ = true;
    return true;
}

std::string PluginRegistryService::manifestPathFor(const std::string& dir_path) const {
    return dir_path + "/plugin.json";
}

void PluginRegistryService::registerPlugin(const std::string& dir_path,
                                           domain::PluginSourceType source_type) {
    auto manifest_path = manifestPathFor(dir_path);
    if (!fs::exists(manifest_path)) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        "plugin.json not found at: " + manifest_path);
        return;
    }

    std::ifstream file(manifest_path);
    if (!file.is_open()) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        "Cannot open plugin.json at: " + manifest_path);
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        std::string("JSON parse error: ") + e.what());
        return;
    }

    domain::PluginManifest manifest;
    try {
        manifest = domain::PluginManifest::from_json(j);
    } catch (const std::exception& e) {
        recordDiagnostic(fs::path(dir_path).filename().string(),
                        "MANIFEST_PARSE_ERROR",
                        std::string("Manifest parse error: ") + e.what());
        return;
    }

    auto errors = manifest.validate();
    if (!errors.empty()) {
        std::string all_errors;
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0) all_errors += "; ";
            all_errors += errors[i];
        }
        recordDiagnostic(manifest.id, "MANIFEST_PARSE_ERROR", all_errors);
    }

    domain::PluginDescriptor desc;
    desc.id = manifest.id;
    desc.name = manifest.name;
    desc.version = manifest.version;
    desc.api_version = manifest.plugin_api_version;
    desc.path = dir_path;
    desc.type = manifest.preferred_process_model;
    desc.source_type = source_type;
    desc.enabled = true;

    plugins_.push_back(std::move(desc));
}

void PluginRegistryService::recordDiagnostic(const std::string& id,
                                             const std::string& code,
                                             const std::string& message) {
    diagnostics_.push_back({id, code, message});
}

void PluginRegistryService::scanBundledDirectory() {
    if (!fs::exists(bundled_plugins_dir_) || !fs::is_directory(bundled_plugins_dir_)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(bundled_plugins_dir_)) {
        if (!entry.is_directory()) continue;

        auto plugin_json = entry.path() / "plugin.json";
        if (fs::exists(plugin_json)) {
            registerPlugin(entry.path().string(), domain::PluginSourceType::BUNDLED);
        }
    }
}

void PluginRegistryService::scanLinkedDirectories() {
    for (const auto& linked_path : linked_config_.paths()) {
        if (!fs::exists(linked_path) || !fs::is_directory(linked_path)) {
            recordDiagnostic(linked_path, "CONFIG_INVALID",
                            "Linked plugin directory does not exist: " + linked_path);
            continue;
        }

        auto plugin_json = fs::path(linked_path) / "plugin.json";
        if (!fs::exists(plugin_json)) {
            recordDiagnostic(linked_path, "MANIFEST_PARSE_ERROR",
                            "plugin.json not found in linked directory: " + linked_path);
            continue;
        }

        registerPlugin(linked_path, domain::PluginSourceType::LINKED);
    }
}

bool PluginRegistryService::addLinkedDirectory(const std::string& path) {
    auto abs_path = fs::absolute(path).string();

    auto plugin_json = fs::path(abs_path) / "plugin.json";
    if (!fs::exists(plugin_json)) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        "plugin.json not found at: " + plugin_json.string());
        return false;
    }

    std::ifstream file(plugin_json);
    if (!file.is_open()) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        "Cannot open: " + plugin_json.string());
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        std::string("JSON parse error: ") + e.what());
        return false;
    }

    domain::PluginManifest manifest;
    try {
        manifest = domain::PluginManifest::from_json(j);
    } catch (const std::exception& e) {
        recordDiagnostic(abs_path, "MANIFEST_PARSE_ERROR",
                        std::string("Manifest parse error: ") + e.what());
        return false;
    }

    auto errors = manifest.validate();
    if (!errors.empty()) {
        std::string all_errors;
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0) all_errors += "; ";
            all_errors += errors[i];
        }
        recordDiagnostic(manifest.id, "MANIFEST_PARSE_ERROR", all_errors);
        return false;
    }

    linked_config_.add(abs_path);
    linked_config_.save();

    pending_restart_ = true;
    return true;
}

bool PluginRegistryService::removeLinkedDirectory(const std::string& path) {
    linked_config_.remove(path);
    linked_config_.save();
    pending_restart_ = true;
    return true;
}

void PluginRegistryService::enablePlugin(const std::string& id) {
    for (auto& p : plugins_) {
        if (p.id == id) {
            if (!p.enabled) {
                p.enabled = true;
                pending_restart_ = true;
            }
            return;
        }
    }
}

void PluginRegistryService::disablePlugin(const std::string& id) {
    for (auto& p : plugins_) {
        if (p.id == id) {
            if (p.enabled) {
                p.enabled = false;
                pending_restart_ = true;
            }
            return;
        }
    }
}

std::vector<domain::PluginDescriptor> PluginRegistryService::discoverAll() {
    return getPlugins();
}

bool PluginRegistryService::isPendingRestart() const {
    return pending_restart_;
}

std::vector<domain::PluginDescriptor> PluginRegistryService::getPlugins() const {
    std::vector<domain::PluginDescriptor> enabled;
    for (const auto& p : plugins_) {
        if (p.enabled) {
            enabled.push_back(p);
        }
    }
    return enabled;
}

std::vector<domain::PluginSource> PluginRegistryService::getSources() const {
    std::vector<domain::PluginSource> sources;
    for (const auto& p : plugins_) {
        domain::PluginSource src;
        src.source_id = p.id;
        src.source_name = p.name;
        src.source_type = p.source_type;
        src.plugin_path = p.path;
        src.plugin_version = p.version;
        src.plugin_api_version = p.api_version;
        src.enabled = p.enabled;
        src.diagnostics_state = domain::PluginDiagnosticsState::OK;

        for (const auto& d : diagnostics_) {
            if (d.plugin_id == p.id) {
                src.diagnostics_state = domain::PluginDiagnosticsState::ERROR;
                break;
            }
        }
        if (!p.enabled) {
            src.diagnostics_state = domain::PluginDiagnosticsState::DISABLED;
        }

        sources.push_back(std::move(src));
    }
    return sources;
}

const std::vector<PluginRegistryService::Diagnostics>&
PluginRegistryService::getDiagnostics() const {
    return diagnostics_;
}

} // namespace micecam::infrastructure

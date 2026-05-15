#pragma once

#include <string>
#include <vector>

#include "domain/PluginDescriptor.h"
#include "domain/PluginManifest.h"
#include "domain/PluginSource.h"
#include "infrastructure/LinkedPluginConfig.h"

namespace micecam::infrastructure {

class PluginRegistryService {
public:
    struct Diagnostics {
        std::string plugin_id;
        std::string error_code;
        std::string message;
    };

    PluginRegistryService(const std::string& bundled_plugins_dir,
                         const std::string& config_dir);

    bool initialize();
    bool addLinkedDirectory(const std::string& path);
    bool removeLinkedDirectory(const std::string& path);
    void enablePlugin(const std::string& id);
    void disablePlugin(const std::string& id);
    std::vector<domain::PluginDescriptor> discoverAll();
    bool isPendingRestart() const;
    std::vector<domain::PluginDescriptor> getPlugins() const;
    std::vector<domain::PluginSource> getSources() const;
    const std::vector<Diagnostics>& getDiagnostics() const;

private:
    std::string bundled_plugins_dir_;
    LinkedPluginConfig linked_config_;
    std::vector<domain::PluginDescriptor> plugins_;
    std::vector<Diagnostics> diagnostics_;
    bool pending_restart_ = false;
    bool initialized_ = false;

    void scanBundledDirectory();
    void scanLinkedDirectories();
    void registerPlugin(const std::string& dir_path,
                        domain::PluginSourceType source_type);
    void recordDiagnostic(const std::string& id, const std::string& code,
                         const std::string& message);
    std::string manifestPathFor(const std::string& dir_path) const;
};

} // namespace micecam::infrastructure

#pragma once

#include <string>
#include <vector>

namespace micecam::infrastructure {

struct LinkedPluginEntry {
    std::string path;
    bool enabled = true;
    std::string added_at;
};

class LinkedPluginConfig {
public:
    explicit LinkedPluginConfig(const std::string& config_path);

    bool load();
    bool save() const;
    void add(const std::string& path);
    void remove(const std::string& path);
    std::vector<std::string> paths() const;

    const std::vector<LinkedPluginEntry>& entries() const;
    bool set_enabled(const std::string& path, bool enabled);

private:
    std::string config_path_;
    std::vector<LinkedPluginEntry> entries_;
};

} // namespace micecam::infrastructure

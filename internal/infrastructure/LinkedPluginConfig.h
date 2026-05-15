#pragma once

#include <string>
#include <vector>

namespace micecam::infrastructure {

class LinkedPluginConfig {
public:
    explicit LinkedPluginConfig(const std::string& config_path);

    bool load();
    bool save() const;
    void add(const std::string& path);
    void remove(const std::string& path);
    std::vector<std::string> paths() const;

private:
    std::string config_path_;
    std::vector<std::string> paths_;
};

} // namespace micecam::infrastructure

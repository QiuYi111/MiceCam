#include "infrastructure/oak_runtime_session.h"

#include <iostream>

int main() {
    micecam::OAKRuntimeSession session;
    micecam::OAKSessionConfig config;
    config.width = 1280;
    config.height = 800;
    config.fps = 30.0;

    std::cout << "oak_runtime_probe: initialize begin\n";
    const bool ok = session.initialize(config);
    std::cout << "oak_runtime_probe: initialize result=" << (ok ? "true" : "false") << "\n";
    if(!ok) {
        return 1;
    }

    auto group = session.get_group();
    std::cout << "oak_runtime_probe: first group=" << (group ? "ok" : "null") << "\n";
    session.stop();
    return group ? 0 : 1;
}

#include <spdlog/spdlog.h>

#include <iostream>
#include <string>

void run_oak_spike();
void run_encoder_spike(bool force_fallback);

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);

    std::string spike_type;
    bool force_fallback = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--spike" && i + 1 < argc) {
            spike_type = argv[++i];
        } else if (arg == "--force-fallback") {
            force_fallback = true;
        }
    }

    if (spike_type.empty()) {
        spdlog::error("Usage: micecam_spike --spike <oak|encoder> [--force-fallback]");
        return 1;
    }

    if (spike_type == "oak") {
        run_oak_spike();
    } else if (spike_type == "encoder") {
        run_encoder_spike(force_fallback);
    } else {
        spdlog::error("Unknown spike type: {}", spike_type);
        spdlog::error("Usage: micecam_spike --spike <oak|encoder> [--force-fallback]");
        return 1;
    }

    return 0;
}

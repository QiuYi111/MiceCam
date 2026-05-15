#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace micecam::domain {

struct StreamSlot {
    uint32_t slot_count = 0;
    uint32_t slot_size = 0;
};

struct ResourceRequest {
    std::string preferred_process_model;
    std::string fallback_process_model;
    uint32_t stream_count = 0;
    std::vector<StreamSlot> stream_slots;
    uint64_t estimated_input_bandwidth = 0;
    uint64_t estimated_output_bandwidth = 0;
    uint32_t encoder_slots_needed = 0;
    uint64_t preview_budget = 0;
};

} // namespace micecam::domain

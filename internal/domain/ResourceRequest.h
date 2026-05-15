#pragma once

#include <cstdint>
#include <optional>
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

enum class AllocationPolicy {
    NO_DROP,
    LATEST_FRAME
};

enum class ProcessModel {
    SINGLETON,
    PER_DEVICE,
    PER_STREAM
};

struct StreamAllocationRequest {
    std::string stream_id;
    std::string plugin_id;
    std::string device_id;
    std::optional<std::string> exclusive_resource_id;
    ProcessModel preferred_process_model = ProcessModel::PER_DEVICE;
    uint32_t min_slot_count = 4;
    uint32_t min_slot_size = 0;
    uint32_t encoder_slots_needed = 1;
    bool is_recording = true;
};

struct RingAllocation {
    std::string stream_id;
    uint32_t slot_count = 0;
    uint32_t slot_size = 0;
    AllocationPolicy policy = AllocationPolicy::NO_DROP;
};

struct AllocationDecision {
    bool accepted = false;
    std::string stream_id;
    std::string reason;
    RingAllocation ring;
    ProcessModel resolved_process_model = ProcessModel::PER_DEVICE;
};

struct GlobalResourceBudget {
    uint32_t max_total_streams = 16;
    uint32_t max_encoder_slots = 8;
    uint64_t max_shm_bytes = 512ULL * 1024 * 1024;
    ProcessModel enforced_process_model = ProcessModel::PER_DEVICE;
    bool override_process_model = false;
};

} // namespace micecam::domain

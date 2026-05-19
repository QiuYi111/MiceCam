#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain/ResourceRequest.h"

namespace micecam::infrastructure {

// Caller contract (session orchestrator):
//   1. Call allocate() with StreamAllocationRequest per device/stream.
//   2. For each accepted AllocationDecision, create a PluginStreamConsumer
//      using decision.ring.slot_count/slot_size as PluginStreamConfig.
//   3. After consumer starts, call
//      RecordingPipeline::set_plugin_source(info.to_json()) and
//      RecordingPipeline::set_stream_transport_stats(stream_id, stats.to_json()).
//   4. On session stop, call release() or release_all().
//   This keeps ownership of the wiring in the session layer, not in ResourceManager.

class ResourceManager {
public:
    explicit ResourceManager(const domain::GlobalResourceBudget& budget = {});

    std::vector<domain::AllocationDecision> allocate(
        const std::vector<domain::StreamAllocationRequest>& requests);

    void release(const std::string& stream_id);

    void release_all();

    bool is_allocated(const std::string& stream_id) const;

    bool is_resource_locked(const std::string& exclusive_resource_id) const;

    uint32_t active_stream_count() const;
    uint32_t active_encoder_slots() const;
    uint64_t active_shm_bytes() const;

private:
    bool check_conflicts(const domain::StreamAllocationRequest& req,
                         std::string& reason) const;

    bool check_budget(const domain::StreamAllocationRequest& req,
                      uint32_t resolved_slot_count,
                      uint32_t resolved_slot_size,
                      std::string& reason) const;

    domain::RingAllocation resolve_ring(
        const domain::StreamAllocationRequest& req) const;

    domain::ProcessModel resolve_process_model(
        const domain::StreamAllocationRequest& req) const;

    void apply_allocation(const domain::AllocationDecision& decision,
                          const domain::StreamAllocationRequest& req);

    struct ActiveAllocation {
        std::string stream_id;
        std::string plugin_id;
        std::string device_id;
        std::optional<std::string> exclusive_resource_id;
        domain::RingAllocation ring;
        domain::ProcessModel process_model;
        uint32_t encoder_slots = 1;
    };

    domain::GlobalResourceBudget budget_;
    std::unordered_map<std::string, ActiveAllocation> allocations_;
    std::unordered_set<std::string> locked_resources_;
};

} // namespace micecam::infrastructure

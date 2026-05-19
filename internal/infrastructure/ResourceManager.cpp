#include "infrastructure/ResourceManager.h"

#include <algorithm>
#include <sstream>

namespace micecam::infrastructure {

ResourceManager::ResourceManager(const domain::GlobalResourceBudget& budget)
    : budget_(budget) {}

std::vector<domain::AllocationDecision> ResourceManager::allocate(
    const std::vector<domain::StreamAllocationRequest>& requests) {
    std::vector<domain::AllocationDecision> decisions;
    decisions.reserve(requests.size());

    for (const auto& req : requests) {
        domain::AllocationDecision decision;
        decision.stream_id = req.stream_id;

        if (is_allocated(req.stream_id)) {
            decision.accepted = false;
            decision.reason = "stream_id already allocated: " + req.stream_id;
            decisions.push_back(decision);
            continue;
        }

        std::string conflict_reason;
        if (!check_conflicts(req, conflict_reason)) {
            decision.accepted = false;
            decision.reason = conflict_reason;
            decisions.push_back(decision);
            continue;
        }

        auto ring = resolve_ring(req);

        std::string budget_reason;
        if (!check_budget(req, ring.slot_count, ring.slot_size, budget_reason)) {
            decision.accepted = false;
            decision.reason = budget_reason;
            decisions.push_back(decision);
            continue;
        }

        decision.accepted = true;
        decision.ring = ring;
        decision.resolved_process_model = resolve_process_model(req);
        decisions.push_back(decision);

        apply_allocation(decision, req);
    }

    return decisions;
}

void ResourceManager::release(const std::string& stream_id) {
    auto it = allocations_.find(stream_id);
    if (it == allocations_.end()) return;

    if (it->second.exclusive_resource_id.has_value()) {
        locked_resources_.erase(it->second.exclusive_resource_id.value());
    }
    allocations_.erase(it);
}

void ResourceManager::release_all() {
    allocations_.clear();
    locked_resources_.clear();
}

bool ResourceManager::is_allocated(const std::string& stream_id) const {
    return allocations_.count(stream_id) > 0;
}

bool ResourceManager::is_resource_locked(
    const std::string& exclusive_resource_id) const {
    return locked_resources_.count(exclusive_resource_id) > 0;
}

uint32_t ResourceManager::active_stream_count() const {
    return static_cast<uint32_t>(allocations_.size());
}

uint32_t ResourceManager::active_encoder_slots() const {
    uint32_t total = 0;
    for (const auto& [_, alloc] : allocations_) {
        total += alloc.encoder_slots;
    }
    return total;
}

uint64_t ResourceManager::active_shm_bytes() const {
    uint64_t total = 0;
    for (const auto& [_, alloc] : allocations_) {
        total += static_cast<uint64_t>(alloc.ring.slot_count) *
                 alloc.ring.slot_size;
    }
    return total;
}

bool ResourceManager::check_conflicts(
    const domain::StreamAllocationRequest& req,
    std::string& reason) const {
    if (!req.exclusive_resource_id.has_value()) return true;

    if (locked_resources_.count(req.exclusive_resource_id.value()) > 0) {
        reason = "exclusive_resource_id conflict: " +
                 req.exclusive_resource_id.value();
        return false;
    }
    return true;
}

bool ResourceManager::check_budget(
    const domain::StreamAllocationRequest& req,
    uint32_t resolved_slot_count,
    uint32_t resolved_slot_size,
    std::string& reason) const {
    uint32_t new_streams = active_stream_count() + 1;
    if (new_streams > budget_.max_total_streams) {
        reason = "stream budget exceeded: " +
                 std::to_string(new_streams) + " > " +
                 std::to_string(budget_.max_total_streams);
        return false;
    }

    uint32_t new_encoders = active_encoder_slots() + req.encoder_slots_needed;
    if (new_encoders > budget_.max_encoder_slots) {
        reason = "encoder budget exceeded: " +
                 std::to_string(new_encoders) + " > " +
                 std::to_string(budget_.max_encoder_slots);
        return false;
    }

    uint64_t ring_bytes =
        static_cast<uint64_t>(resolved_slot_count) * resolved_slot_size;
    uint64_t new_shm = active_shm_bytes() + ring_bytes;
    if (new_shm > budget_.max_shm_bytes) {
        reason = "SHM budget exceeded";
        return false;
    }

    return true;
}

domain::RingAllocation ResourceManager::resolve_ring(
    const domain::StreamAllocationRequest& req) const {
    domain::RingAllocation ring;
    ring.stream_id = req.stream_id;

    uint32_t slot_size = req.min_slot_size;
    if (slot_size == 0) {
        slot_size = req.is_recording ? 4194304u : 1048576u;
    }

    uint32_t slot_count = req.min_slot_count;
    if (slot_count == 0) {
        slot_count = 4;
    }

    ring.slot_count = slot_count;
    ring.slot_size = slot_size;

    if (req.is_recording) {
        ring.policy = domain::AllocationPolicy::NO_DROP;
    } else {
        ring.policy = domain::AllocationPolicy::LATEST_FRAME;
    }

    return ring;
}

domain::ProcessModel ResourceManager::resolve_process_model(
    const domain::StreamAllocationRequest& req) const {
    if (budget_.override_process_model) {
        return budget_.enforced_process_model;
    }
    return req.preferred_process_model;
}

void ResourceManager::apply_allocation(
    const domain::AllocationDecision& decision,
    const domain::StreamAllocationRequest& req) {
    ActiveAllocation alloc;
    alloc.stream_id = req.stream_id;
    alloc.plugin_id = req.plugin_id;
    alloc.device_id = req.device_id;
    alloc.exclusive_resource_id = req.exclusive_resource_id;
    alloc.ring = decision.ring;
    alloc.process_model = decision.resolved_process_model;
    alloc.encoder_slots = req.encoder_slots_needed;

    if (req.exclusive_resource_id.has_value()) {
        locked_resources_.insert(req.exclusive_resource_id.value());
    }

    allocations_[req.stream_id] = std::move(alloc);
}

} // namespace micecam::infrastructure

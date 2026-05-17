#include <gtest/gtest.h>

#include "infrastructure/ResourceManager.h"

using namespace micecam;
using namespace micecam::domain;
using namespace micecam::infrastructure;

namespace {

StreamAllocationRequest makeRecording(const std::string& stream_id,
                                       const std::string& plugin_id,
                                       const std::string& device_id) {
    StreamAllocationRequest req;
    req.stream_id = stream_id;
    req.plugin_id = plugin_id;
    req.device_id = device_id;
    req.is_recording = true;
    req.min_slot_count = 8;
    req.min_slot_size = 4194304;
    req.encoder_slots_needed = 1;
    req.preferred_process_model = ProcessModel::PER_DEVICE;
    return req;
}

StreamAllocationRequest makePreview(const std::string& stream_id,
                                     const std::string& plugin_id,
                                     const std::string& device_id) {
    StreamAllocationRequest req;
    req.stream_id = stream_id;
    req.plugin_id = plugin_id;
    req.device_id = device_id;
    req.is_recording = false;
    req.min_slot_count = 4;
    req.min_slot_size = 1048576;
    req.encoder_slots_needed = 0;
    req.preferred_process_model = ProcessModel::PER_DEVICE;
    return req;
}

StreamAllocationRequest makeExclusive(const std::string& stream_id,
                                       const std::string& plugin_id,
                                       const std::string& device_id,
                                       const std::string& exclusive_id) {
    auto req = makeRecording(stream_id, plugin_id, device_id);
    req.exclusive_resource_id = exclusive_id;
    return req;
}

} // namespace

TEST(ResourceManagerTest, SingleRecordingAllocationAccepted) {
    ResourceManager rm;
    auto req = makeRecording("s1", "p1", "d1");
    auto decisions = rm.allocate({req});

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].stream_id, "s1");
    EXPECT_EQ(decisions[0].ring.policy, AllocationPolicy::NO_DROP);
    EXPECT_EQ(decisions[0].ring.slot_count, 8u);
    EXPECT_EQ(decisions[0].ring.slot_size, 4194304u);
}

TEST(ResourceManagerTest, SinglePreviewAllocationAccepted) {
    ResourceManager rm;
    auto req = makePreview("s1", "p1", "d1");
    auto decisions = rm.allocate({req});

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].ring.policy, AllocationPolicy::LATEST_FRAME);
}

TEST(ResourceManagerTest, RecordingUsesNoDropPolicy) {
    ResourceManager rm;
    auto req = makeRecording("s1", "p1", "d1");
    auto decisions = rm.allocate({req});

    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].ring.policy, AllocationPolicy::NO_DROP);
}

TEST(ResourceManagerTest, PreviewUsesLatestFramePolicy) {
    ResourceManager rm;
    auto req = makePreview("s1", "p1", "d1");
    auto decisions = rm.allocate({req});

    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].ring.policy, AllocationPolicy::LATEST_FRAME);
}

TEST(ResourceManagerTest, ExclusiveConflictRejected) {
    ResourceManager rm;
    auto req1 = makeExclusive("s1", "p1", "d1", "usb_bus_1");
    auto req2 = makeExclusive("s2", "p1", "d2", "usb_bus_1");
    auto decisions = rm.allocate({req1, req2});

    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_FALSE(decisions[1].accepted);
    EXPECT_FALSE(decisions[1].reason.empty());
    EXPECT_NE(decisions[1].reason.find("conflict"), std::string::npos);
}

TEST(ResourceManagerTest, NonConflictingDevicesAllocatedTogether) {
    ResourceManager rm;
    auto req1 = makeExclusive("s1", "p1", "d1", "usb_bus_1");
    auto req2 = makeExclusive("s2", "p1", "d2", "usb_bus_2");
    auto decisions = rm.allocate({req1, req2});

    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_TRUE(decisions[1].accepted);
}

TEST(ResourceManagerTest, ExclusiveConflictAfterRelease) {
    ResourceManager rm;
    auto req1 = makeExclusive("s1", "p1", "d1", "usb_bus_1");
    rm.allocate({req1});

    rm.release("s1");
    EXPECT_FALSE(rm.is_resource_locked("usb_bus_1"));

    auto req2 = makeExclusive("s2", "p1", "d2", "usb_bus_1");
    auto decisions = rm.allocate({req2});
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].accepted);
}

TEST(ResourceManagerTest, DuplicateStreamIdRejected) {
    ResourceManager rm;
    auto req1 = makeRecording("s1", "p1", "d1");
    rm.allocate({req1});

    auto req2 = makeRecording("s1", "p1", "d1");
    auto decisions = rm.allocate({req2});
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_NE(decisions[0].reason.find("already allocated"), std::string::npos);
}

TEST(ResourceManagerTest, StreamBudgetExceeded) {
    GlobalResourceBudget budget;
    budget.max_total_streams = 2;
    ResourceManager rm(budget);

    auto req1 = makeRecording("s1", "p1", "d1");
    auto req2 = makeRecording("s2", "p1", "d2");
    auto req3 = makeRecording("s3", "p1", "d3");

    auto decisions = rm.allocate({req1, req2, req3});
    ASSERT_EQ(decisions.size(), 3u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_TRUE(decisions[1].accepted);
    EXPECT_FALSE(decisions[2].accepted);
    EXPECT_NE(decisions[2].reason.find("stream budget"), std::string::npos);
}

TEST(ResourceManagerTest, EncoderBudgetExceeded) {
    GlobalResourceBudget budget;
    budget.max_encoder_slots = 2;
    ResourceManager rm(budget);

    auto req1 = makeRecording("s1", "p1", "d1");
    req1.encoder_slots_needed = 2;
    auto req2 = makeRecording("s2", "p1", "d2");
    req2.encoder_slots_needed = 1;

    auto decisions = rm.allocate({req1, req2});
    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_FALSE(decisions[1].accepted);
    EXPECT_NE(decisions[1].reason.find("encoder budget"), std::string::npos);
}

TEST(ResourceManagerTest, ShmBudgetExceeded) {
    GlobalResourceBudget budget;
    budget.max_shm_bytes = 1024;
    ResourceManager rm(budget);

    auto req = makeRecording("s1", "p1", "d1");
    req.min_slot_count = 1;
    req.min_slot_size = 2048;

    auto decisions = rm.allocate({req});
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_NE(decisions[0].reason.find("SHM budget"), std::string::npos);
}

TEST(ResourceManagerTest, ProcessModelOverride) {
    GlobalResourceBudget budget;
    budget.override_process_model = true;
    budget.enforced_process_model = ProcessModel::SINGLETON;
    ResourceManager rm(budget);

    auto req = makeRecording("s1", "p1", "d1");
    req.preferred_process_model = ProcessModel::PER_STREAM;

    auto decisions = rm.allocate({req});
    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].resolved_process_model, ProcessModel::SINGLETON);
}

TEST(ResourceManagerTest, ProcessModelNotOverriddenWhenDisabled) {
    GlobalResourceBudget budget;
    budget.override_process_model = false;
    ResourceManager rm(budget);

    auto req = makeRecording("s1", "p1", "d1");
    req.preferred_process_model = ProcessModel::PER_STREAM;

    auto decisions = rm.allocate({req});
    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].resolved_process_model, ProcessModel::PER_STREAM);
}

TEST(ResourceManagerTest, RingRespectsMinSlotCount) {
    ResourceManager rm;
    auto req = makeRecording("s1", "p1", "d1");
    req.min_slot_count = 16;

    auto decisions = rm.allocate({req});
    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].ring.slot_count, 16u);
}

TEST(ResourceManagerTest, RingRespectsMinSlotSize) {
    ResourceManager rm;
    auto req = makeRecording("s1", "p1", "d1");
    req.min_slot_size = 8388608;

    auto decisions = rm.allocate({req});
    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].ring.slot_size, 8388608u);
}

TEST(ResourceManagerTest, DefaultRingSizeRecording) {
    ResourceManager rm;
    StreamAllocationRequest req;
    req.stream_id = "s1";
    req.plugin_id = "p1";
    req.device_id = "d1";
    req.is_recording = true;
    req.min_slot_count = 0;
    req.min_slot_size = 0;

    auto decisions = rm.allocate({req});
    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].ring.policy, AllocationPolicy::NO_DROP);
    EXPECT_EQ(decisions[0].ring.slot_count, 4u);
    EXPECT_EQ(decisions[0].ring.slot_size, 4194304u);
}

TEST(ResourceManagerTest, DefaultRingSizePreview) {
    ResourceManager rm;
    StreamAllocationRequest req;
    req.stream_id = "s1";
    req.plugin_id = "p1";
    req.device_id = "d1";
    req.is_recording = false;
    req.min_slot_count = 0;
    req.min_slot_size = 0;

    auto decisions = rm.allocate({req});
    ASSERT_TRUE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].ring.policy, AllocationPolicy::LATEST_FRAME);
    EXPECT_EQ(decisions[0].ring.slot_count, 4u);
    EXPECT_EQ(decisions[0].ring.slot_size, 1048576u);
}

TEST(ResourceManagerTest, RejectionReasonIsStructured) {
    ResourceManager rm;
    auto req1 = makeExclusive("s1", "p1", "d1", "cam_0");
    rm.allocate({req1});

    auto req2 = makeExclusive("s2", "p1", "d2", "cam_0");
    auto decisions = rm.allocate({req2});
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_EQ(decisions[0].stream_id, "s2");
    EXPECT_FALSE(decisions[0].reason.empty());
    EXPECT_NE(decisions[0].reason.find("cam_0"), std::string::npos);
}

TEST(ResourceManagerTest, ReleaseAll) {
    ResourceManager rm;
    auto req1 = makeRecording("s1", "p1", "d1");
    auto req2 = makeRecording("s2", "p1", "d2");
    rm.allocate({req1, req2});

    EXPECT_EQ(rm.active_stream_count(), 2u);
    rm.release_all();
    EXPECT_EQ(rm.active_stream_count(), 0u);
    EXPECT_EQ(rm.active_encoder_slots(), 0u);
    EXPECT_EQ(rm.active_shm_bytes(), 0u);
}

TEST(ResourceManagerTest, ActiveEncoderSlotsTracking) {
    ResourceManager rm;
    auto req1 = makeRecording("s1", "p1", "d1");
    req1.encoder_slots_needed = 3;
    rm.allocate({req1});

    EXPECT_EQ(rm.active_encoder_slots(), 3u);
}

TEST(ResourceManagerTest, ActiveShmBytesTracking) {
    ResourceManager rm;
    auto req = makeRecording("s1", "p1", "d1");
    req.min_slot_count = 8;
    req.min_slot_size = 4194304;
    rm.allocate({req});

    uint64_t expected = 8u * 4194304u;
    EXPECT_EQ(rm.active_shm_bytes(), expected);
}

TEST(ResourceManagerTest, IsAllocated) {
    ResourceManager rm;
    EXPECT_FALSE(rm.is_allocated("s1"));

    auto req = makeRecording("s1", "p1", "d1");
    rm.allocate({req});
    EXPECT_TRUE(rm.is_allocated("s1"));
}

TEST(ResourceManagerTest, ReleaseUnallocatedIsHarmless) {
    ResourceManager rm;
    rm.release("nonexistent");
    EXPECT_EQ(rm.active_stream_count(), 0u);
}

TEST(ResourceManagerTest, MixedRecordingAndPreview) {
    ResourceManager rm;
    auto rec = makeRecording("rec_1", "p1", "d1");
    auto prev = makePreview("prev_1", "p1", "d1");

    auto decisions = rm.allocate({rec, prev});
    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_TRUE(decisions[1].accepted);
    EXPECT_EQ(decisions[0].ring.policy, AllocationPolicy::NO_DROP);
    EXPECT_EQ(decisions[1].ring.policy, AllocationPolicy::LATEST_FRAME);
}

TEST(ResourceManagerTest, NoExclusiveIdNeverConflicts) {
    ResourceManager rm;
    auto req1 = makeRecording("s1", "p1", "d1");
    auto req2 = makeRecording("s2", "p1", "d2");

    auto decisions = rm.allocate({req1, req2});
    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_TRUE(decisions[1].accepted);
}

TEST(ResourceManagerTest, ExclusiveWithNonExclusive) {
    ResourceManager rm;
    auto excl = makeExclusive("s1", "p1", "d1", "cam_0");
    auto non_excl = makeRecording("s2", "p2", "d2");

    auto decisions = rm.allocate({excl, non_excl});
    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_TRUE(decisions[1].accepted);
}

TEST(ResourceManagerTest, BatchAllocationStopsOnFirstConflict) {
    ResourceManager rm;
    auto req1 = makeExclusive("s1", "p1", "d1", "cam_0");
    auto req2 = makeExclusive("s2", "p1", "d2", "cam_0");
    auto req3 = makeRecording("s3", "p1", "d3");

    auto decisions = rm.allocate({req1, req2, req3});
    ASSERT_EQ(decisions.size(), 3u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_FALSE(decisions[1].accepted);
    EXPECT_TRUE(decisions[2].accepted);
}

TEST(ResourceManagerTest, AllocationDecisionDefaultState) {
    domain::AllocationDecision d;
    EXPECT_FALSE(d.accepted);
    EXPECT_TRUE(d.reason.empty());
    EXPECT_EQ(d.ring.slot_count, 0u);
    EXPECT_EQ(d.ring.slot_size, 0u);
    EXPECT_EQ(d.ring.policy, AllocationPolicy::NO_DROP);
    EXPECT_EQ(d.resolved_process_model, ProcessModel::PER_DEVICE);
}

TEST(ResourceManagerTest, ReleaseRestoresEncoderBudget) {
    GlobalResourceBudget budget;
    budget.max_encoder_slots = 2;
    ResourceManager rm(budget);

    auto req1 = makeRecording("s1", "p1", "d1");
    req1.encoder_slots_needed = 2;
    rm.allocate({req1});
    EXPECT_EQ(rm.active_encoder_slots(), 2u);

    rm.release("s1");
    EXPECT_EQ(rm.active_encoder_slots(), 0u);

    auto req2 = makeRecording("s2", "p1", "d2");
    req2.encoder_slots_needed = 2;
    auto decisions = rm.allocate({req2});
    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].accepted);
}

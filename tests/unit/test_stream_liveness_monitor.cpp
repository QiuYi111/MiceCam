#include <gtest/gtest.h>

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "infrastructure/StreamLivenessMonitor.h"

using namespace micecam;
using namespace std::chrono_literals;

namespace {

struct StallEvent {
    std::string stream_id;
    std::string plugin_id;
    uint64_t stall_duration_ms;
    int stall_count;
};

struct AllStalledEvent {
    std::string plugin_id;
};

struct ClockState {
    std::chrono::steady_clock::time_point now;
};

class StreamLivenessMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        clock_state_.now = std::chrono::steady_clock::now();
        monitor_ = std::make_unique<infrastructure::StreamLivenessMonitor>(
            stall_timeout_ms_,
            [this]() { return clock_state_.now; }
        );
        monitor_->set_stall_callback([this](const std::string& sid, const std::string& pid, uint64_t ms, int count) {
            std::lock_guard<std::mutex> lock(events_mutex_);
            stall_events_.push_back({sid, pid, ms, count});
        });
        monitor_->set_all_stalled_callback([this](const std::string& pid) {
            std::lock_guard<std::mutex> lock(events_mutex_);
            all_stalled_events_.push_back({pid});
        });
    }

    void TearDown() override {
        if (monitor_) monitor_->stop();
    }

    void advance_clock(std::chrono::milliseconds ms) {
        clock_state_.now += ms;
    }

    void wait_for_monitor_cycle() {
        std::this_thread::sleep_for(1500ms);
    }

    static constexpr uint64_t stall_timeout_ms_ = 5000;
    ClockState clock_state_;
    std::unique_ptr<infrastructure::StreamLivenessMonitor> monitor_;

    std::mutex events_mutex_;
    std::vector<StallEvent> stall_events_;
    std::vector<AllStalledEvent> all_stalled_events_;
};

TEST_F(StreamLivenessMonitorTest, StallCallbackFiresAfterTimeout) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    ASSERT_FALSE(stall_events_.empty());
    EXPECT_EQ(stall_events_[0].stream_id, "cam1");
    EXPECT_EQ(stall_events_[0].plugin_id, "plugin_a");
}

TEST_F(StreamLivenessMonitorTest, NoStallCallbackBeforeTimeout) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ - 1000));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    EXPECT_TRUE(stall_events_.empty());
    EXPECT_TRUE(all_stalled_events_.empty());
}

TEST_F(StreamLivenessMonitorTest, PartialPluginStallNoAllStalled) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->register_stream("cam2", "plugin_a");
    monitor_->register_stream("cam3", "plugin_b");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    monitor_->update_activity("cam2");
    monitor_->update_activity("cam3");
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    auto cam1_stall = std::any_of(stall_events_.begin(), stall_events_.end(),
        [](const StallEvent& e) { return e.stream_id == "cam1"; });
    EXPECT_TRUE(cam1_stall);
    auto cam2_stall = std::any_of(stall_events_.begin(), stall_events_.end(),
        [](const StallEvent& e) { return e.stream_id == "cam2"; });
    EXPECT_FALSE(cam2_stall);
    EXPECT_TRUE(all_stalled_events_.empty());
}

TEST_F(StreamLivenessMonitorTest, AllStreamsStalledFiresAllStalledCallback) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->register_stream("cam2", "plugin_a");
    monitor_->register_stream("cam3", "plugin_b");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    auto plugin_a_all = std::any_of(all_stalled_events_.begin(), all_stalled_events_.end(),
        [](const AllStalledEvent& e) { return e.plugin_id == "plugin_a"; });
    EXPECT_TRUE(plugin_a_all);

    auto plugin_b_all = std::any_of(all_stalled_events_.begin(), all_stalled_events_.end(),
        [](const AllStalledEvent& e) { return e.plugin_id == "plugin_b"; });
    EXPECT_TRUE(plugin_b_all);
}

TEST_F(StreamLivenessMonitorTest, UnregisterRemovesStream) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->unregister_stream("cam1");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    EXPECT_TRUE(stall_events_.empty());
}

TEST_F(StreamLivenessMonitorTest, UpdateActivityResetsTimer) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ - 1000));
    monitor_->update_activity("cam1");
    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ - 1000));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    EXPECT_TRUE(stall_events_.empty());
}

TEST_F(StreamLivenessMonitorTest, StallCountIncrementsPerCycle) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    wait_for_monitor_cycle();
    advance_clock(std::chrono::milliseconds(2000));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    ASSERT_GE(stall_events_.size(), 2u);
    EXPECT_EQ(stall_events_[0].stall_count, 1);
    EXPECT_EQ(stall_events_[1].stall_count, 2);
}

TEST_F(StreamLivenessMonitorTest, StallCountResetsOnActivity) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    wait_for_monitor_cycle();

    {
        std::lock_guard<std::mutex> lock(events_mutex_);
        ASSERT_FALSE(stall_events_.empty());
        EXPECT_EQ(stall_events_.back().stall_count, 1);
    }

    monitor_->update_activity("cam1");
    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    auto last = stall_events_.back();
    EXPECT_EQ(last.stall_count, 1);
}

TEST_F(StreamLivenessMonitorTest, StallCountResetsWhenStreamRecoversInMonitorLoop) {
    monitor_->register_stream("cam1", "plugin_a");
    monitor_->start();

    advance_clock(std::chrono::milliseconds(stall_timeout_ms_ + 100));
    wait_for_monitor_cycle();

    {
        std::lock_guard<std::mutex> lock(events_mutex_);
        ASSERT_FALSE(stall_events_.empty());
        EXPECT_EQ(stall_events_.back().stall_count, 1);
    }

    monitor_->update_activity("cam1");
    advance_clock(std::chrono::milliseconds(2000));
    wait_for_monitor_cycle();

    monitor_->stop();

    std::lock_guard<std::mutex> lock(events_mutex_);
    EXPECT_GE(stall_events_.size(), 1u);
    auto last = stall_events_.back();
    EXPECT_EQ(last.stall_count, 1);
}

} // namespace

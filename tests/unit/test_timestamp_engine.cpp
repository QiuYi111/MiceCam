#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "domain/TimestampEngine.h"

using namespace micecam::domain;

TEST(TimestampEngineTest, PopulateSetsWallTimeNs) {
    TimestampEngine engine;
    engine.capture_wall_anchor();

    auto frame_time = std::chrono::steady_clock::now();
    auto ts = engine.populate(frame_time);

    EXPECT_GT(ts.wall_time_ns, 0u);
    EXPECT_FALSE(ts.has_hardware_pts);
    EXPECT_EQ(ts.hardware_pts, 0u);
}

TEST(TimestampEngineTest, PopulateWallTimeAdvances) {
    TimestampEngine engine;
    engine.capture_wall_anchor();

    auto t1 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto t2 = std::chrono::steady_clock::now();

    auto ts1 = engine.populate(t1);
    auto ts2 = engine.populate(t2);

    EXPECT_GT(ts2.wall_time_ns, ts1.wall_time_ns);
    EXPECT_GT(ts2.session_offset_us, ts1.session_offset_us);
}

TEST(TimestampEngineTest, WithHardwarePtsLeavesWallTimeZero) {
    TimestampEngine engine;
    engine.capture_wall_anchor();

    auto ts = engine.with_hardware_pts(12345);
    EXPECT_TRUE(ts.has_hardware_pts);
    EXPECT_EQ(ts.hardware_pts, 12345u);
    EXPECT_EQ(ts.wall_time_ns, 0u);
}

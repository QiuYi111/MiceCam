#include <gtest/gtest.h>

#include "domain/AlertRecord.h"
#include "pipeline/StatsCollector.h"

using namespace micecam;

TEST(StatsCollector, RecordFrameIncrementsCounters) {
    pipeline::StatsCollector collector("test_stream");
    collector.start(1000000);

    collector.record_frame(0, 0, 1500.0, 33333);
    collector.record_frame(1, 1, 1450.0, 33333);
    collector.record_frame(2, 2, 1600.0, 33334);

    auto stats = collector.finalize();
    EXPECT_EQ(stats.frames_expected, 3u);
    EXPECT_EQ(stats.frames_actual, 3u);
    EXPECT_DOUBLE_EQ(stats.drop_rate, 0.0);
}

TEST(StatsCollector, FrameDropCalculatedCorrectly) {
    pipeline::StatsCollector collector("test_stream");
    collector.start(1000000);

    collector.record_frame(0, 0, 1500.0, 33333);
    collector.record_frame(1, 1, 1500.0, 33333);
    collector.record_frame(2, 2, 1500.0, 33333);
    collector.record_frame(3, 3, 1500.0, 33333);
    collector.record_frame(4, 4, 1500.0, 33333);

    auto stats = collector.finalize();
    EXPECT_EQ(stats.frames_expected, 5u);
    EXPECT_EQ(stats.frames_actual, 5u);
}

TEST(StatsCollector, EncodeLatencyStatsCorrect) {
    pipeline::StatsCollector collector("test_stream");
    collector.start(1000000);

    collector.record_frame(0, 0, 1000.0, 33333);
    collector.record_frame(1, 1, 2000.0, 33333);
    collector.record_frame(2, 2, 3000.0, 33333);

    auto stats = collector.finalize();
    EXPECT_NEAR(stats.avg_encode_latency_us, 2000.0, 1.0);
    EXPECT_NEAR(stats.min_encode_latency_us, 1000.0, 1.0);
    EXPECT_NEAR(stats.max_encode_latency_us, 3000.0, 1.0);
}

TEST(StatsCollector, AlertAddedCorrectly) {
    pipeline::StatsCollector collector("test_stream");
    collector.start(1000000);

    domain::AlertRecord alert;
    alert.type = domain::AlertType::HIGH_DROP_RATE;
    alert.severity = domain::AlertSeverity::YELLOW;
    alert.stream_id = "test_stream";
    alert.message = "Drop rate exceeded 0.1%";

    collector.add_alert(alert);
    auto stats = collector.finalize();
    EXPECT_EQ(stats.stream_id, "test_stream");
}

TEST(StatsCollector, StreamIdCorrect) {
    pipeline::StatsCollector collector("cam_a");
    collector.start(33333);
    auto stats = collector.finalize();
    EXPECT_EQ(stats.stream_id, "cam_a");
}

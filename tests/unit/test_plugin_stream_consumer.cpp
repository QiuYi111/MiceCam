#include <gtest/gtest.h>

#include "infrastructure/PluginStreamConsumer.h"
#include "pipeline/RecordingPipeline.h"

using namespace micecam;
using namespace micecam::infrastructure;
using namespace micecam::pipeline;

namespace {

PluginStreamConfig makeConfig() {
    PluginStreamConfig cfg;
    cfg.plugin_id = "test_plugin";
    cfg.device_id = "dev0";
    cfg.stream_id = "stream_0";
    cfg.shm_name = "/micecam_test_consumer_nonexistent";
    cfg.slot_count = 4;
    cfg.slot_size = 4096;
    return cfg;
}

} // anonymous namespace

TEST(PluginStreamConsumerTest, GetPluginSourceInfo) {
    RecordingPipeline pipeline;
    auto config = makeConfig();
    PluginStreamConsumer consumer(pipeline, config);

    auto info = consumer.getPluginSourceInfo();
    EXPECT_EQ(info.plugin_id, "test_plugin");
    EXPECT_EQ(info.device_id, "dev0");
    EXPECT_EQ(info.transport, "posix_shm");
    EXPECT_EQ(info.ring_slot_count, 4u);
    EXPECT_EQ(info.ring_slot_size, 4096u);
}

TEST(PluginStreamConsumerTest, InitialTransportStatsZeroed) {
    RecordingPipeline pipeline;
    auto config = makeConfig();
    PluginStreamConsumer consumer(pipeline, config);

    auto stats = consumer.getTransportStats();
    EXPECT_EQ(stats.frames_read, 0u);
    EXPECT_EQ(stats.frames_dropped, 0u);
    EXPECT_EQ(stats.backpressure_events, 0u);
    EXPECT_DOUBLE_EQ(stats.avg_consumer_lag, 0.0);
    EXPECT_DOUBLE_EQ(stats.max_consumer_lag, 0.0);
}

TEST(PluginStreamConsumerTest, StartFailsOnInvalidRing) {
    RecordingPipeline pipeline;
    auto config = makeConfig();
    PluginStreamConsumer consumer(pipeline, config);

    EXPECT_FALSE(consumer.start());
}

TEST(PluginStreamConsumerTest, StopWithoutStartIsHarmless) {
    RecordingPipeline pipeline;
    auto config = makeConfig();
    PluginStreamConsumer consumer(pipeline, config);

    consumer.stop();
    auto stats = consumer.getTransportStats();
    EXPECT_EQ(stats.frames_read, 0u);
}

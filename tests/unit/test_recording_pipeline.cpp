#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "domain/EncoderConfig.h"
#include "domain/StreamConfig.h"
#include "infrastructure/AlertManager.h"
#include "infrastructure/Watchdog.h"
#include "pipeline/RecordingPipeline.h"

using namespace micecam;

TEST(RecordingPipeline, StartSetsStateToRunning) {
    pipeline::SessionConfig config;
    config.session_id = "test_session";
    config.output_dir = "/tmp/micecam_test";

    pipeline::RecordingPipeline pipeline;
    EXPECT_TRUE(pipeline.start(config));
}

TEST(RecordingPipeline, StartWithEmptyStreams) {
    pipeline::SessionConfig config;
    config.session_id = "test_empty";
    config.output_dir = "/tmp/micecam_test";

    pipeline::RecordingPipeline pipeline;
    EXPECT_TRUE(pipeline.start(config));
    pipeline.stop();
}

TEST(RecordingPipeline, PushFrameWithoutStartFails) {
    pipeline::RecordingPipeline pipeline;

    pipeline::FrameData frame;
    frame.stream_id = "stream_a";
    EXPECT_FALSE(pipeline.push_frame(frame));
}

TEST(RecordingPipeline, StopTransitionsToFinalized) {
    pipeline::SessionConfig config;
    config.session_id = "test_stop";
    config.output_dir = "/tmp/micecam_test";

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));
    pipeline.stop();
    auto [meta, stats] = pipeline.result();
    EXPECT_FALSE(meta.session_id.empty());
}

TEST(RecordingPipeline, ResultReturnsMetadata) {
    pipeline::SessionConfig config;
    config.session_id = "test_result";
    config.output_dir = "/tmp/micecam_test";

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));
    pipeline.stop();

    auto [meta, stats] = pipeline.result();
    EXPECT_EQ(meta.session_id, "test_result");
}

TEST(RecordingPipeline, SecondStartFails) {
    pipeline::SessionConfig config;
    config.session_id = "test_double";
    config.output_dir = "/tmp/micecam_test";

    pipeline::RecordingPipeline pipeline;
    EXPECT_TRUE(pipeline.start(config));
    EXPECT_FALSE(pipeline.start(config));
    pipeline.stop();
}

TEST(RecordingPipeline, WatchdogIntegration) {
    infrastructure::AlertManager alert_mgr;
    infrastructure::Watchdog watchdog(alert_mgr);

    pipeline::SessionConfig config;
    config.session_id = "test_wd";
    config.output_dir = "/tmp/micecam_test";

    pipeline::RecordingPipeline pipeline;
    pipeline.set_watchdog(&watchdog);

    ASSERT_TRUE(pipeline.start(config));

    pipeline::FrameData frame;
    frame.stream_id = "nonexistent";
    pipeline.push_frame(frame);

    pipeline.stop();
}

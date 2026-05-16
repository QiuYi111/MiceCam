#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "domain/CalibrationResult.h"
#include "domain/EncoderConfig.h"
#include "domain/StreamConfig.h"
#include "infrastructure/AlertManager.h"
#include "infrastructure/Watchdog.h"
#include "pipeline/RecordingPipeline.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>

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

TEST(RecordingPipeline, H264PassthroughSkipsTranscode) {
    pipeline::SessionConfig config;
    config.session_id = "test_h264_pass";
    config.output_dir = "/tmp/micecam_test";

    domain::StreamConfig sc;
    sc.device_id = "cam0";
    sc.stream_index = 0;
    sc.width = 640;
    sc.height = 480;
    sc.framerate = 30;
    config.streams.push_back(sc);

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    std::vector<uint8_t> fake_h264_data = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x1f};

    pipeline::FrameData frame;
    frame.stream_id = "cam0_0";
    frame.data = fake_h264_data.data();
    frame.size = fake_h264_data.size();
    frame.width = 640;
    frame.height = 480;
    frame.pts = 0;
    frame.source_format = "h264";
    frame.payload_kind = pipeline::PayloadKind::H264;
    frame.is_keyframe = true;

    EXPECT_TRUE(pipeline.push_frame(frame));

    frame.payload_kind = pipeline::PayloadKind::H264;
    frame.is_keyframe = false;
    frame.pts = 33333;
    EXPECT_TRUE(pipeline.push_frame(frame));

    pipeline.stop();
}

TEST(RecordingPipeline, H265PassthroughSkipsTranscode) {
    pipeline::SessionConfig config;
    config.session_id = "test_h265_pass";
    config.output_dir = "/tmp/micecam_test";

    domain::StreamConfig sc;
    sc.device_id = "cam1";
    sc.stream_index = 0;
    sc.width = 640;
    sc.height = 480;
    sc.framerate = 30;
    config.streams.push_back(sc);

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    std::vector<uint8_t> fake_h265_data = {0x00, 0x00, 0x00, 0x01, 0x40, 0x01};

    pipeline::FrameData frame;
    frame.stream_id = "cam1_0";
    frame.data = fake_h265_data.data();
    frame.size = fake_h265_data.size();
    frame.width = 640;
    frame.height = 480;
    frame.pts = 0;
    frame.source_format = "h265";
    frame.payload_kind = pipeline::PayloadKind::H265;
    frame.is_keyframe = true;

    EXPECT_TRUE(pipeline.push_frame(frame));
    pipeline.stop();
}

TEST(RecordingPipeline, RAWFallbackUsesTranscodeStage) {
    pipeline::SessionConfig config;
    config.session_id = "test_raw_fallback";
    config.output_dir = "/tmp/micecam_test";
    config.encoder.keyframe_interval = 30;

    domain::StreamConfig sc;
    sc.device_id = "cam2";
    sc.stream_index = 0;
    sc.width = 320;
    sc.height = 240;
    sc.framerate = 30;
    config.streams.push_back(sc);

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    std::vector<uint8_t> raw_data(320 * 240 * 3, 128);

    pipeline::FrameData frame;
    frame.stream_id = "cam2_0";
    frame.data = raw_data.data();
    frame.size = raw_data.size();
    frame.width = 320;
    frame.height = 240;
    frame.pts = 0;
    frame.source_format = "rgb24";
    frame.payload_kind = pipeline::PayloadKind::RAW;

    EXPECT_TRUE(pipeline.push_frame(frame));
    pipeline.stop();
}

TEST(RecordingPipeline, CalibrationResultSetsFallbackGOP) {
    pipeline::SessionConfig config;
    config.session_id = "test_calib_gop";
    config.output_dir = "/tmp/micecam_test";

    domain::StreamConfig sc;
    sc.device_id = "cam3";
    sc.stream_index = 0;
    sc.width = 320;
    sc.height = 240;
    sc.framerate = 30;
    config.streams.push_back(sc);

    domain::CalibrationResult cal;
    cal.stream_id = "cam3_0";
    cal.min_gop = 15;
    cal.success = true;
    config.calibration_results["cam3_0"] = cal;

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    std::vector<uint8_t> raw_data(320 * 240 * 3, 100);
    pipeline::FrameData frame;
    frame.stream_id = "cam3_0";
    frame.data = raw_data.data();
    frame.size = raw_data.size();
    frame.width = 320;
    frame.height = 240;
    frame.pts = 0;
    frame.source_format = "rgb24";
    frame.payload_kind = pipeline::PayloadKind::RAW;

    EXPECT_TRUE(pipeline.push_frame(frame));
    pipeline.stop();
}

TEST(RecordingPipeline, OverflowWarningLogged) {
    pipeline::SessionConfig config;
    config.session_id = "test_overflow";
    config.output_dir = "/tmp/micecam_test";

    domain::StreamConfig sc;
    sc.device_id = "cam4";
    sc.stream_index = 0;
    sc.width = 320;
    sc.height = 240;
    sc.framerate = 30;
    config.streams.push_back(sc);

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    std::vector<uint8_t> raw_data(320 * 240 * 3, 128);
    pipeline::FrameData frame;
    frame.stream_id = "cam4_0";
    frame.data = raw_data.data();
    frame.size = raw_data.size();
    frame.width = 320;
    frame.height = 240;
    frame.pts = 0;
    frame.source_format = "rgb24";
    frame.payload_kind = pipeline::PayloadKind::RAW;
    frame.dropped_frame_count = 5;

    std::ostringstream oss;
    auto oss_sink = std::make_shared<spdlog::sinks::ostream_sink_st>(oss);
    auto test_logger = std::make_shared<spdlog::logger>("test_overflow", oss_sink);
    auto prev_default = spdlog::default_logger();
    spdlog::set_default_logger(test_logger);

    EXPECT_TRUE(pipeline.push_frame(frame));

    spdlog::set_default_logger(prev_default);
    spdlog::drop("test_overflow");

    std::string log_output = oss.str();
    EXPECT_NE(log_output.find("ring buffer overflow"), std::string::npos);
    EXPECT_NE(log_output.find("5 frames dropped"), std::string::npos);

    EXPECT_EQ(pipeline.get_overflow_count("cam4_0"), 5u);

    pipeline.stop();
}

TEST(RecordingPipeline, OverflowCountAccumulates) {
    pipeline::SessionConfig config;
    config.session_id = "test_overflow_accum";
    config.output_dir = "/tmp/micecam_test";

    domain::StreamConfig sc;
    sc.device_id = "cam5";
    sc.stream_index = 0;
    sc.width = 320;
    sc.height = 240;
    sc.framerate = 30;
    config.streams.push_back(sc);

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    std::vector<uint8_t> raw_data(320 * 240 * 3, 128);

    for (int i = 0; i < 3; i++) {
        pipeline::FrameData frame;
        frame.stream_id = "cam5_0";
        frame.data = raw_data.data();
        frame.size = raw_data.size();
        frame.width = 320;
        frame.height = 240;
        frame.pts = i * 33333;
        frame.source_format = "rgb24";
        frame.payload_kind = pipeline::PayloadKind::RAW;
        frame.dropped_frame_count = 2;
        EXPECT_TRUE(pipeline.push_frame(frame));
    }

    EXPECT_EQ(pipeline.get_overflow_count("cam5_0"), 6u);

    pipeline.stop();
}

TEST(RecordingPipeline, OverflowCountZeroForUnknownStream) {
    pipeline::RecordingPipeline pipeline;
    EXPECT_EQ(pipeline.get_overflow_count("nonexistent"), 0u);
}

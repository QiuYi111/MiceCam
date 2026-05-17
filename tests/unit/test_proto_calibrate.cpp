#include <gtest/gtest.h>

#include "micecam/camera_plugin.pb.h"

TEST(ProtoCalibrateTest, CalibrateRequestSerializesDeserializes) {
    micecam::plugin::CalibrateRequest req;
    req.set_device_id("dev_0");
    req.set_stream_index(1);
    req.set_width(1920);
    req.set_height(1080);
    req.set_fps(30.0);
    req.set_requested_payload(micecam::plugin::PayloadKind::H264);
    req.set_pixel_format("nv12");
    req.set_prefer_hardware_encoder(true);
    req.set_bitrate_kbps(5000);
    req.set_crf(23);
    req.set_max_b_frames(2);
    req.set_calibration_duration_ms(5000);
    (*req.mutable_config())["key"] = "value";

    std::string serialized;
    ASSERT_TRUE(req.SerializeToString(&serialized));

    micecam::plugin::CalibrateRequest parsed;
    ASSERT_TRUE(parsed.ParseFromString(serialized));

    EXPECT_EQ(parsed.device_id(), "dev_0");
    EXPECT_EQ(parsed.stream_index(), 1);
    EXPECT_EQ(parsed.width(), 1920);
    EXPECT_EQ(parsed.height(), 1080);
    EXPECT_DOUBLE_EQ(parsed.fps(), 30.0);
    EXPECT_EQ(parsed.requested_payload(), micecam::plugin::PayloadKind::H264);
    EXPECT_EQ(parsed.pixel_format(), "nv12");
    EXPECT_TRUE(parsed.prefer_hardware_encoder());
    EXPECT_EQ(parsed.bitrate_kbps(), 5000);
    EXPECT_EQ(parsed.crf(), 23);
    EXPECT_EQ(parsed.max_b_frames(), 2);
    EXPECT_EQ(parsed.calibration_duration_ms(), 5000);
    EXPECT_EQ(parsed.config().at("key"), "value");
}

TEST(ProtoCalibrateTest, CalibrateResponseSerializesDeserializes) {
    micecam::plugin::CalibrateResponse resp;
    resp.set_success(true);
    resp.set_i_frame_latency_ns(1000000);
    resp.set_p_frame_latency_ns(500000);
    resp.set_supported(true);
    resp.set_actual_encoder_name("h264_videotoolbox");
    resp.set_actual_width(1920);
    resp.set_actual_height(1080);
    resp.set_max_sustainable_fps(60.0);
    resp.set_recommended_slot_size(262144);
    resp.add_warnings("high bitrate");

    std::string serialized;
    ASSERT_TRUE(resp.SerializeToString(&serialized));

    micecam::plugin::CalibrateResponse parsed;
    ASSERT_TRUE(parsed.ParseFromString(serialized));

    EXPECT_TRUE(parsed.success());
    EXPECT_EQ(parsed.i_frame_latency_ns(), 1000000u);
    EXPECT_EQ(parsed.p_frame_latency_ns(), 500000u);
    EXPECT_TRUE(parsed.supported());
    EXPECT_EQ(parsed.actual_encoder_name(), "h264_videotoolbox");
    EXPECT_EQ(parsed.actual_width(), 1920);
    EXPECT_EQ(parsed.actual_height(), 1080);
    EXPECT_DOUBLE_EQ(parsed.max_sustainable_fps(), 60.0);
    EXPECT_EQ(parsed.recommended_slot_size(), 262144u);
    ASSERT_EQ(parsed.warnings_size(), 1);
    EXPECT_EQ(parsed.warnings(0), "high bitrate");
}

TEST(ProtoCalibrateTest, CalibrateRequestDefaultDuration) {
    micecam::plugin::CalibrateRequest req;
    EXPECT_EQ(req.calibration_duration_ms(), 0);
}

TEST(ProtoCalibrateTest, StreamConfigHasKeyframeInterval) {
    micecam::plugin::StreamConfig config;
    config.set_device_id("dev_0");
    config.set_keyframe_interval(30);

    std::string serialized;
    ASSERT_TRUE(config.SerializeToString(&serialized));

    micecam::plugin::StreamConfig parsed;
    ASSERT_TRUE(parsed.ParseFromString(serialized));

    EXPECT_EQ(parsed.device_id(), "dev_0");
    EXPECT_EQ(parsed.keyframe_interval(), 30);
}

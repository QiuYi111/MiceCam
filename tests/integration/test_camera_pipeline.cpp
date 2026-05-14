#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>

#include "domain/CameraStream.h"
#include "domain/EncoderConfig.h"
#include "domain/StreamConfig.h"
#include "infrastructure/CameraManager.h"
#include "infrastructure/MockCameraBackend.h"
#include "infrastructure/AlertManager.h"
#include "infrastructure/Watchdog.h"
#include "pipeline/RecordingPipeline.h"

using namespace micecam;

TEST(CameraPipelineIntegration, MockCameraToRecordingPipeline) {
    auto backend = std::make_unique<infrastructure::MockCameraBackend>();
    backend->set_drop_every_n(0);

    infrastructure::CameraManager mgr;
    mgr.register_backend(std::move(backend));

    auto devices = mgr.discover_all();
    ASSERT_GE(devices.size(), 1u);

    infrastructure::AlertManager alert_mgr;
    infrastructure::Watchdog watchdog(alert_mgr);

    pipeline::SessionConfig config;
    config.session_id = "integration_test";
    config.output_dir = "/tmp/micecam_integration";

    std::error_code ec;
    std::filesystem::create_directories("/tmp/micecam_integration", ec);

    domain::StreamConfig sc;
    sc.device_id = "mock_cam_0";
    sc.stream_index = 0;
    sc.width = 640;
    sc.height = 480;
    sc.framerate = 30;
    sc.pixel_format = "rgb24";
    config.streams.push_back(sc);

    domain::EncoderConfig enc;
    enc.bitrate_kbps = 5000;
    enc.prefer_hardware = false;
    config.encoder = enc;

    pipeline::RecordingPipeline pipeline;
    pipeline.set_watchdog(&watchdog);
    pipeline.set_alert_manager(&alert_mgr);

    ASSERT_TRUE(pipeline.start(config));

    auto stream = mgr.open_stream(sc);
    ASSERT_NE(stream, nullptr);

    int frame_count = 0;
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(5);

    while (std::chrono::steady_clock::now() - start < timeout) {
        std::vector<uint8_t> data;
        int64_t pts;
        if (stream->read_frame(data, pts)) {
            pipeline::FrameData frame;
            frame.stream_id = sc.device_id + "_0";
            frame.data = data.data();
            frame.size = data.size();
            frame.width = 640;
            frame.height = 480;
            frame.pts = pts;
            frame.source_format = "rgb24";
            pipeline.push_frame(frame);
            frame_count++;
        }
        if (frame_count >= 30) break;
    }

    pipeline.stop();
    auto [meta, stats] = pipeline.result();

    EXPECT_GE(frame_count, 1);
    EXPECT_EQ(meta.session_id, "integration_test");
    EXPECT_GE(stats.size(), 0u);

    std::filesystem::remove_all("/tmp/micecam_integration/integration_test", ec);
}

TEST(CameraPipelineIntegration, MultipleStreamsStart) {
    infrastructure::CameraManager mgr;
    for (int i = 0; i < 2; i++) {
        mgr.register_backend(std::make_unique<infrastructure::MockCameraBackend>());
    }

    pipeline::SessionConfig config;
    config.session_id = "multi_stream";
    config.output_dir = "/tmp/micecam_integration";

    for (int i = 0; i < 2; i++) {
        domain::StreamConfig sc;
        sc.device_id = "mock_cam_0";
        sc.stream_index = i;
        sc.width = 320;
        sc.height = 240;
        sc.framerate = 30;
        config.streams.push_back(sc);
    }

    pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));
    pipeline.stop();
    auto [meta, stats] = pipeline.result();

    EXPECT_GE(stats.size(), 2u);

    std::error_code ec;
    std::filesystem::remove_all("/tmp/micecam_integration/multi_stream", ec);
}

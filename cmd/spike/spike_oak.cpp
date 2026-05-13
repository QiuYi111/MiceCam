#include <spdlog/spdlog.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#ifdef HAS_DEPTHAI
#include <depthai/depthai.hpp>
#endif

void run_oak_spike() {
    spdlog::info("=== OAK H264 Spike ===");

#ifndef HAS_DEPTHAI
    spdlog::warn("HAS_DEPTHAI not defined. depthai-core is unavailable.");
    spdlog::warn("Skipping OAK Part A: depthai-core submodule not initialized.");
    spdlog::warn("To enable, run: git submodule update --init 3rdParty/depthai-core");
    spdlog::info("=== OAK H264 Spike: SKIPPED ===");
    return;
#else
    spdlog::info("Checking for OAK-D device...");

    bool device_found = false;
    {
        auto device_infos = dai::Device::getAllAvailableDevices();
        if (device_infos.empty()) {
            spdlog::warn("No OAK-D device detected via USB.");
            spdlog::warn("Skipping OAK Part A: physical device not connected.");
            spdlog::info("=== OAK H264 Spike: SKIPPED (no device) ===");
            return;
        }
        spdlog::info("Found {} device(s)", device_infos.size());
        for (const auto& info : device_infos) {
            spdlog::info("  Device: {} (state: {})", info.getMxId(), static_cast<int>(info.state));
        }
        device_found = true;
    }

    if (!device_found) {
        spdlog::info("=== OAK H264 Spike: SKIPPED ===");
        return;
    }

    try {
        dai::Pipeline pipeline;

        auto cam = pipeline.create<dai::node::ColorCamera>();
        cam->setBoardSocket(dai::CameraBoardSocket::CAM_A);
        cam->setResolution(dai::ColorCameraProperties::SensorResolution::THE_800_P);
        cam->setFps(30);
        cam->setInterleaved(false);

        auto encoder = pipeline.create<dai::node::VideoEncoder>();
        encoder->setDefaultProfilePreset(
            30,
            dai::VideoEncoderProperties::Profile::H264_MAIN
        );
        encoder->setKeyframeFrequency(30);
        encoder->setBitrate(4000000);

        cam->video.link(encoder->input);

        auto out_queue = encoder->bitstream.createOutputQueue(8, false);

        spdlog::info("Starting pipeline...");
        pipeline.start();

        std::ofstream outfile("spike_oak.h264", std::ios::binary);
        if (!outfile) {
            spdlog::error("Failed to open spike_oak.h264 for writing");
            return;
        }

        spdlog::info("Recording H264 for 10 seconds...");
        auto start = std::chrono::steady_clock::now();
        int frame_count = 0;

        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
            auto frame = out_queue->tryGet<dai::ImgFrame>();
            if (frame) {
                const auto& data = frame->getData();
                outfile.write(reinterpret_cast<const char*>(data.data()), data.size());

                spdlog::info("Frame #{}: seq={}, size={} bytes, ts={}",
                    frame_count,
                    frame->getSequenceNum(),
                    data.size(),
                    frame->getTimestamp().time_since_epoch().count()
                );
                frame_count++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        outfile.close();
        pipeline.stop();

        spdlog::info("=== OAK H264 Spike: COMPLETE ===");
        spdlog::info("Total frames: {}", frame_count);
        spdlog::info("Output: spike_oak.h264");
        spdlog::info("Verify: ffprobe spike_oak.h264");

    } catch (const std::exception& e) {
        spdlog::error("OAK spike exception: {}", e.what());
        spdlog::info("=== OAK H264 Spike: FAILED ===");
    }
#endif
}

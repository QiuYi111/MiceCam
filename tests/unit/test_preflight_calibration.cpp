#include <gtest/gtest.h>
#include <cmath>
#include <map>
#include <string>

#include "domain/StreamConfig.h"
#include "pipeline/PreflightValidator.h"

using namespace micecam;

#ifdef _WIN32
constexpr const char* TEST_TMP = ".";
#else
constexpr const char* TEST_TMP = "/tmp";
#endif

class MockCalibrationClient : public pipeline::ICalibrationClient {
public:
    domain::CalibrationResult calibrate(
        const std::string& device_id, int stream_index,
        int width, int height, double fps) override {
        call_count++;
        last_width = width;
        last_height = height;
        if (fail_first_ && call_count == 1) {
            domain::CalibrationResult fail_result;
            fail_result.success = false;
            return fail_result;
        }
        return mock_result_;
    }

    void setMockResult(const domain::CalibrationResult& r) { mock_result_ = r; }
    void setFailFirst(bool v) { fail_first_ = v; }
    int callCount() const { return call_count; }
    int lastWidth() const { return last_width; }
    int lastHeight() const { return last_height; }

private:
    domain::CalibrationResult mock_result_;
    bool fail_first_ = false;
    int call_count = 0;
    int last_width = 0;
    int last_height = 0;
};

class MockStreamTestController : public pipeline::IStreamTestController {
public:
    bool openStream(const domain::StreamConfig& config) override {
        auto sid = config.device_id + ":" + std::to_string(config.stream_index);
        opened_.push_back(sid);
        last_configs_[sid] = config;
        return !fail_open_.count(sid);
    }

    void closeStream(const std::string& stream_id) override {
        closed_.push_back(stream_id);
    }

    uint64_t getDropCount(const std::string& stream_id) override {
        auto it = drop_counts_.find(stream_id);
        return it != drop_counts_.end() ? it->second : 0;
    }

    void setDropCount(const std::string& sid, uint64_t count) {
        drop_counts_[sid] = count;
    }

    void setFailOpen(const std::string& sid) { fail_open_.insert(sid); }

    const std::vector<std::string>& opened() const { return opened_; }
    const std::vector<std::string>& closed() const { return closed_; }
    const domain::StreamConfig& lastConfig(const std::string& sid) const { return last_configs_.at(sid); }

private:
    std::vector<std::string> opened_;
    std::vector<std::string> closed_;
    std::map<std::string, domain::StreamConfig> last_configs_;
    std::map<std::string, uint64_t> drop_counts_;
    std::set<std::string> fail_open_;
};

static domain::StreamConfig makeConfig(const std::string& dev, int idx,
                                        int w, int h, int fps) {
    domain::StreamConfig c;
    c.device_id = dev;
    c.stream_index = idx;
    c.width = w;
    c.height = h;
    c.framerate = fps;
    return c;
}

TEST(PreflightCalibration, MinGopComputation_BasicCase) {
    int gop = pipeline::PreflightValidator::compute_min_gop(2'000'000, 7'000'000, 30.0);
    EXPECT_EQ(gop, 1);
}

TEST(PreflightCalibration, MinGopComputation_HigherI) {
    int gop = pipeline::PreflightValidator::compute_min_gop(50'000'000, 5'000'000, 30.0);
    EXPECT_EQ(gop, 2);
}

TEST(PreflightCalibration, MinGopComputation_LargeGop) {
    int gop = pipeline::PreflightValidator::compute_min_gop(100'000'000, 5'000'000, 30.0);
    double frame_interval = 1e9 / 30.0;
    double divisor = frame_interval - 5'000'000.0;
    int expected = static_cast<int>(std::ceil(100'000'000.0 / divisor));
    EXPECT_EQ(gop, expected);
}

TEST(PreflightCalibration, BlocksWhenPExceedsFrameInterval) {
    int gop = pipeline::PreflightValidator::compute_min_gop(2'000'000, 34'000'000, 30.0);
    EXPECT_EQ(gop, -1);
}

TEST(PreflightCalibration, BlocksWhenPIsExactlyFrameInterval) {
    double frame_interval_ns = 1e9 / 30.0;
    uint64_t p_ns = static_cast<uint64_t>(std::ceil(frame_interval_ns));
    int gop = pipeline::PreflightValidator::compute_min_gop(2'000'000, p_ns, 30.0);
    EXPECT_EQ(gop, -1);
}

TEST(PreflightCalibration, BlocksWhenFpsZero) {
    int gop = pipeline::PreflightValidator::compute_min_gop(2'000'000, 7'000'000, 0.0);
    EXPECT_EQ(gop, -1);
}

TEST(PreflightCalibration, Phase1SuccessfulCalibration) {
    MockCalibrationClient client;
    domain::CalibrationResult mock;
    mock.success = true;
    mock.i_frame_latency_ns = 2'000'000;
    mock.p_frame_latency_ns = 7'000'000;
    mock.actual_encoder_name = "h264_vaapi";
    client.setMockResult(mock);

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto results = validator.run_phase1_calibration(configs, &client);

    ASSERT_EQ(results.size(), 1u);
    const auto& cal = results.at("cam0:0");
    EXPECT_TRUE(cal.success);
    EXPECT_EQ(cal.min_gop, 1);
    EXPECT_EQ(cal.stream_id, "cam0:0");
    EXPECT_EQ(cal.actual_encoder_name, "h264_vaapi");
}

TEST(PreflightCalibration, Phase1RetryLowerResolution) {
    MockCalibrationClient client;
    client.setFailFirst(true);
    domain::CalibrationResult mock;
    mock.success = true;
    mock.i_frame_latency_ns = 2'000'000;
    mock.p_frame_latency_ns = 7'000'000;
    client.setMockResult(mock);

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto results = validator.run_phase1_calibration(configs, &client);

    ASSERT_EQ(results.size(), 1u);
    const auto& cal = results.at("cam0:0");
    EXPECT_TRUE(cal.success);
    EXPECT_TRUE(cal.degraded_resolution);
    EXPECT_EQ(client.lastWidth(), 960);
    EXPECT_EQ(client.lastHeight(), 540);
    EXPECT_EQ(client.callCount(), 2);
}

TEST(PreflightCalibration, Phase1RetryStillFails) {
    MockCalibrationClient client;
    client.setFailFirst(true);
    domain::CalibrationResult mock;
    mock.success = false;
    client.setMockResult(mock);

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto results = validator.run_phase1_calibration(configs, &client);

    ASSERT_EQ(results.size(), 1u);
    const auto& cal = results.at("cam0:0");
    EXPECT_FALSE(cal.success);
    EXPECT_TRUE(cal.degraded_resolution);
}

TEST(PreflightCalibration, Phase1BlocksOnHighPLatency) {
    MockCalibrationClient client;
    domain::CalibrationResult mock;
    mock.success = true;
    mock.i_frame_latency_ns = 2'000'000;
    mock.p_frame_latency_ns = 34'000'000;
    client.setMockResult(mock);

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto results = validator.run_phase1_calibration(configs, &client);

    ASSERT_EQ(results.size(), 1u);
    const auto& cal = results.at("cam0:0");
    EXPECT_FALSE(cal.success);
    EXPECT_EQ(cal.min_gop, -1);
}

TEST(PreflightCalibration, Phase2DropDetection) {
    MockStreamTestController controller;
    controller.setDropCount("cam0:0", 42);

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30),
                    makeConfig("cam1", 0, 1280, 720, 30)};
    auto result = validator.run_phase2_stress_test(configs, &controller, 50);

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.drop_counts.at("cam0:0"), 42u);
    EXPECT_EQ(result.drop_counts.at("cam1:0"), 0u);
    ASSERT_EQ(result.warnings.size(), 1u);
    EXPECT_NE(result.warnings[0].find("cam0:0"), std::string::npos);
}

TEST(PreflightCalibration, Phase2NoDrops) {
    MockStreamTestController controller;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.run_phase2_stress_test(configs, &controller, 50);

    EXPECT_TRUE(result.passed);
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_EQ(result.drop_counts.at("cam0:0"), 0u);
}

TEST(PreflightCalibration, Phase2OpensAndClosesAllStreams) {
    MockStreamTestController controller;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30),
                    makeConfig("cam1", 0, 1280, 720, 30)};
    auto result = validator.run_phase2_stress_test(configs, &controller, 50);

    ASSERT_EQ(controller.opened().size(), 2u);
    ASSERT_EQ(controller.closed().size(), 2u);
    EXPECT_EQ(controller.opened()[0], "cam0:0");
    EXPECT_EQ(controller.opened()[1], "cam1:0");
}

TEST(PreflightCalibration, FullValidateWithCalibrationAndStress) {
    MockCalibrationClient cal_client;
    domain::CalibrationResult mock;
    mock.success = true;
    mock.i_frame_latency_ns = 2'000'000;
    mock.p_frame_latency_ns = 7'000'000;
    cal_client.setMockResult(mock);

    MockStreamTestController stream_ctrl;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.validate(configs, TEST_TMP, 60,
                                      &cal_client, &stream_ctrl, 50);

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.calibration_results.size(), 1u);
    EXPECT_TRUE(result.calibration_results.at("cam0:0").success);
}

TEST(PreflightCalibration, FullValidateFailsWhenPhase1Blocked) {
    MockCalibrationClient cal_client;
    domain::CalibrationResult mock;
    mock.success = true;
    mock.i_frame_latency_ns = 2'000'000;
    mock.p_frame_latency_ns = 34'000'000;
    cal_client.setMockResult(mock);

    MockStreamTestController stream_ctrl;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.validate(configs, TEST_TMP, 60,
                                      &cal_client, &stream_ctrl, 50);

    EXPECT_FALSE(result.passed);
    EXPECT_NE(result.message.find("Phase 1 calibration failed"), std::string::npos);
}

TEST(PreflightCalibration, FullValidateWithPhase2Warnings) {
    MockCalibrationClient cal_client;
    domain::CalibrationResult mock;
    mock.success = true;
    mock.i_frame_latency_ns = 2'000'000;
    mock.p_frame_latency_ns = 7'000'000;
    cal_client.setMockResult(mock);

    MockStreamTestController stream_ctrl;
    stream_ctrl.setDropCount("cam0:0", 5);

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.validate(configs, TEST_TMP, 60,
                                      &cal_client, &stream_ctrl, 50);

    EXPECT_TRUE(result.passed);
    EXPECT_FALSE(result.calibration_results.empty());
    ASSERT_FALSE(result.warnings.empty());
    EXPECT_NE(result.warnings[0].find("dropped"), std::string::npos);
}

TEST(PreflightCalibration, KeyframeIntervalPropagatedToOpenStream) {
    MockCalibrationClient cal_client;
    domain::CalibrationResult mock;
    mock.success = true;
    mock.i_frame_latency_ns = 100'000'000;
    mock.p_frame_latency_ns = 5'000'000;
    cal_client.setMockResult(mock);

    MockStreamTestController stream_ctrl;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.validate(configs, TEST_TMP, 60,
                                      &cal_client, &stream_ctrl, 50);

    EXPECT_TRUE(result.passed);
    const auto& cal = result.calibration_results.at("cam0:0");
    ASSERT_TRUE(cal.success);
    EXPECT_GT(cal.min_gop, 0);

    ASSERT_EQ(stream_ctrl.opened().size(), 1u);
    const auto& cfg = stream_ctrl.lastConfig("cam0:0");
    EXPECT_EQ(cfg.keyframe_interval, cal.min_gop);
}

TEST(PreflightCalibration, KeyframeIntervalDefaultWithoutCalibration) {
    MockStreamTestController stream_ctrl;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.validate(configs, TEST_TMP, 60,
                                      nullptr, &stream_ctrl, 50);

    EXPECT_TRUE(result.passed);
    ASSERT_EQ(stream_ctrl.opened().size(), 1u);
    const auto& cfg = stream_ctrl.lastConfig("cam0:0");
    EXPECT_EQ(cfg.keyframe_interval, 0);
}

TEST(PreflightCalibration, KeyframeIntervalFromDirectPhase2Call) {
    domain::CalibrationResult cal;
    cal.success = true;
    cal.min_gop = 15;
    std::map<std::string, domain::CalibrationResult> cal_results;
    cal_results["cam0:0"] = cal;

    MockStreamTestController stream_ctrl;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.run_phase2_stress_test(configs, &stream_ctrl, 50, &cal_results);

    EXPECT_TRUE(result.passed);
    ASSERT_EQ(stream_ctrl.opened().size(), 1u);
    const auto& cfg = stream_ctrl.lastConfig("cam0:0");
    EXPECT_EQ(cfg.keyframe_interval, 15);
}

TEST(PreflightCalibration, KeyframeIntervalZeroWhenCalibrationFailed) {
    domain::CalibrationResult cal;
    cal.success = false;
    cal.min_gop = -1;
    std::map<std::string, domain::CalibrationResult> cal_results;
    cal_results["cam0:0"] = cal;

    MockStreamTestController stream_ctrl;

    pipeline::PreflightValidator validator;
    auto configs = {makeConfig("cam0", 0, 1920, 1080, 30)};
    auto result = validator.run_phase2_stress_test(configs, &stream_ctrl, 50, &cal_results);

    EXPECT_TRUE(result.passed);
    ASSERT_EQ(stream_ctrl.opened().size(), 1u);
    const auto& cfg = stream_ctrl.lastConfig("cam0:0");
    EXPECT_EQ(cfg.keyframe_interval, 0);
}

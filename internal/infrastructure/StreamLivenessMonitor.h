#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace micecam::infrastructure {

using ClockFn = std::function<std::chrono::steady_clock::time_point()>;
using StallCallback = std::function<void(const std::string& stream_id, const std::string& plugin_id, uint64_t stall_duration_ms, int stall_count)>;
using AllStalledCallback = std::function<void(const std::string& plugin_id)>;

class StreamLivenessMonitor {
public:
    explicit StreamLivenessMonitor(
        uint64_t stall_timeout_ms = 5000,
        ClockFn clock = nullptr
    );
    ~StreamLivenessMonitor();

    StreamLivenessMonitor(const StreamLivenessMonitor&) = delete;
    StreamLivenessMonitor& operator=(const StreamLivenessMonitor&) = delete;

    void start();
    void stop();

    void register_stream(const std::string& stream_id, const std::string& plugin_id);
    void unregister_stream(const std::string& stream_id);
    void update_activity(const std::string& stream_id);

    void set_stall_callback(StallCallback cb);
    void set_all_stalled_callback(AllStalledCallback cb);

    int cycle_count() const;

private:
    void monitor_loop();
    ClockFn clock_;
    uint64_t stall_timeout_ms_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<int> cycle_count_{0};
    std::thread monitor_thread_;

    std::mutex mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_active_;
    std::unordered_map<std::string, std::string> stream_to_plugin_;
    std::unordered_map<std::string, int> stall_counts_;
    std::unordered_set<std::string> plugins_all_stalled_fired_;

    StallCallback stall_cb_;
    AllStalledCallback all_stalled_cb_;
};

} // namespace micecam::infrastructure

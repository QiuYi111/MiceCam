#include "infrastructure/StreamLivenessMonitor.h"

#include <algorithm>
#include <unordered_set>

namespace micecam::infrastructure {

StreamLivenessMonitor::StreamLivenessMonitor(uint64_t stall_timeout_ms, ClockFn clock)
    : clock_(clock ? std::move(clock) : std::chrono::steady_clock::now)
    , stall_timeout_ms_(stall_timeout_ms) {}

StreamLivenessMonitor::~StreamLivenessMonitor() {
    stop();
}

void StreamLivenessMonitor::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;
    monitor_thread_ = std::jthread([this](std::stop_token) { monitor_loop(); });
}

void StreamLivenessMonitor::stop() {
    running_.store(false);
    if (monitor_thread_.joinable()) {
        monitor_thread_.request_stop();
        monitor_thread_.join();
    }
}

void StreamLivenessMonitor::register_stream(const std::string& stream_id, const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_active_[stream_id] = clock_();
    stream_to_plugin_[stream_id] = plugin_id;
}

void StreamLivenessMonitor::unregister_stream(const std::string& stream_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_active_.erase(stream_id);
    stream_to_plugin_.erase(stream_id);
}

void StreamLivenessMonitor::update_activity(const std::string& stream_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (last_active_.count(stream_id)) {
        last_active_[stream_id] = clock_();
        auto it = stream_to_plugin_.find(stream_id);
        if (it != stream_to_plugin_.end()) {
            plugins_all_stalled_fired_.erase(it->second);
        }
    }
}

void StreamLivenessMonitor::set_stall_callback(StallCallback cb) {
    stall_cb_ = std::move(cb);
}

void StreamLivenessMonitor::set_all_stalled_callback(AllStalledCallback cb) {
    all_stalled_cb_ = std::move(cb);
}

void StreamLivenessMonitor::monitor_loop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!running_.load()) break;

        auto now = clock_();
        std::lock_guard<std::mutex> lock(mutex_);

        std::unordered_map<std::string, std::unordered_set<std::string>> plugin_streams;
        std::unordered_set<std::string> stalled_streams;

        for (auto& [sid, plugin_id] : stream_to_plugin_) {
            plugin_streams[plugin_id].insert(sid);
            auto it = last_active_.find(sid);
            if (it == last_active_.end()) continue;
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
            if (static_cast<uint64_t>(elapsed_ms) > stall_timeout_ms_) {
                stalled_streams.insert(sid);
                if (stall_cb_) {
                    stall_cb_(sid, plugin_id, static_cast<uint64_t>(elapsed_ms));
                }
            }
        }

        for (auto& [plugin_id, streams] : plugin_streams) {
            bool all_stalled = std::all_of(streams.begin(), streams.end(),
                [&](const std::string& sid) { return stalled_streams.count(sid) > 0; });
            if (all_stalled && !plugins_all_stalled_fired_.count(plugin_id)) {
                plugins_all_stalled_fired_.insert(plugin_id);
                if (all_stalled_cb_) {
                    all_stalled_cb_(plugin_id);
                }
            }
        }
    }
}

} // namespace micecam::infrastructure

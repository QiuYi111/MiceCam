#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace micecam::infrastructure {

class AlertManager;

class Watchdog {
public:
    explicit Watchdog(AlertManager& alert_mgr);
    ~Watchdog();

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

    void set_timeout_s(int seconds);
    void start();
    void stop();
    void feed();

private:
    void monitor_loop();

    AlertManager& alert_mgr_;
    std::atomic<int> timeout_s_{3};
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> last_feed_ns_{0};
    std::thread thread_;
    std::mutex mutex_;
};

} // namespace micecam::infrastructure

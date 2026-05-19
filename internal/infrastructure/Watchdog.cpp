#include "Watchdog.h"

#include "AlertManager.h"
#include "domain/AlertRecord.h"

#include <chrono>

namespace micecam::infrastructure {

Watchdog::Watchdog(AlertManager& alert_mgr) : alert_mgr_(alert_mgr) {
    last_feed_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

Watchdog::~Watchdog() {
    stop();
}

void Watchdog::set_timeout_s(int seconds) {
    timeout_s_ = seconds;
}

void Watchdog::start() {
    running_ = true;
    last_feed_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    thread_ = std::thread(&Watchdog::monitor_loop, this);
}

void Watchdog::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Watchdog::feed() {
    last_feed_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void Watchdog::monitor_loop() {
    while (running_) {
        auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        auto elapsed_ns = now - last_feed_ns_.load();
        auto timeout_ns = static_cast<uint64_t>(timeout_s_.load()) * 1000000000ULL;

        if (elapsed_ns > timeout_ns) {
            domain::AlertRecord alert;
            alert.type = domain::AlertType::PIPELINE_STALL;
            alert.severity = domain::AlertSeverity::RED;
            alert.timestamp_ns = static_cast<uint64_t>(now);
            alert.message = "Pipeline stalled: no feed for >" + std::to_string(timeout_s_.load()) + "s";
            alert_mgr_.emit(alert);

            last_feed_ns_ = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

} // namespace micecam::infrastructure

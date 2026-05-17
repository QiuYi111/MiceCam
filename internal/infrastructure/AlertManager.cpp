#include "AlertManager.h"

namespace micecam::infrastructure {

void AlertManager::register_observer(api::WatchdogObserver* observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    observers_.push_back(observer);
}

void AlertManager::unregister_observer(api::WatchdogObserver* observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(observers_.begin(), observers_.end(), observer);
    if (it != observers_.end()) {
        observers_.erase(it);
    }
}

void AlertManager::emit(const domain::AlertRecord& alert) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_duplicate(alert)) {
        return;
    }

    DedupKey key{alert.type, alert.stream_id};
    last_emit_[key] = std::chrono::steady_clock::now();

    history_.push_back(alert);

    for (auto* obs : observers_) {
        if (obs) {
            obs->on_alert(alert);
        }
    }
}

void AlertManager::set_dedup_cooldown_ms(int ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    dedup_cooldown_ms_ = ms;
}

bool AlertManager::is_duplicate(const domain::AlertRecord& alert) {
    DedupKey key{alert.type, alert.stream_id};
    auto it = last_emit_.find(key);
    if (it == last_emit_.end()) {
        return false;
    }
    auto elapsed = std::chrono::steady_clock::now() - it->second;
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < dedup_cooldown_ms_;
}

std::vector<domain::AlertRecord> AlertManager::history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

void AlertManager::clear_history() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}

} // namespace micecam::infrastructure

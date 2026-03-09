#include "infrastructure/frame_dispatcher.h"
#include <iostream>
#include <algorithm>

namespace micecam {

void FrameDispatcher::attach(std::shared_ptr<IFrameObserver> observer) {
    if (!observer) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already attached
    for (const auto& weak_obs : observers_) {
        if (auto existing = weak_obs.lock()) {
            if (existing == observer) {
                return; // Already attached
            }
        }
    }

    observers_.push_back(observer);
}

void FrameDispatcher::detach(std::shared_ptr<IFrameObserver> observer) {
    if (!observer) return;

    std::lock_guard<std::mutex> lock(mutex_);

    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [&observer](const std::weak_ptr<IFrameObserver>& weak_obs) {
                auto existing = weak_obs.lock();
                return !existing || existing == observer;
            }),
        observers_.end()
    );
}

void FrameDispatcher::dispatch(const FrameView& frame) {
    std::vector<std::shared_ptr<IFrameObserver>> active_observers;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Collect active observers and clean up expired ones
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [&active_observers](const std::weak_ptr<IFrameObserver>& weak_obs) {
                    if (auto obs = weak_obs.lock()) {
                        active_observers.push_back(obs);
                        return false;
                    }
                    return true; // Remove expired
                }),
            observers_.end()
        );
    }

    // Dispatch outside of lock to avoid holding mutex during callbacks
    for (const auto& observer : active_observers) {
        try {
            observer->on_frame(frame);
        } catch (const std::exception& e) {
            std::cerr << "[FrameDispatcher] Observer exception: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[FrameDispatcher] Observer threw unknown exception\n";
        }
    }
}

size_t FrameDispatcher::observer_count() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& weak_obs : observers_) {
        if (weak_obs.lock()) {
            ++count;
        }
    }
    return count;
}

} // namespace micecam

#pragma once

#include "micecam/observer.h"
#include <vector>
#include <memory>
#include <mutex>

namespace micecam {

/**
 * @brief Thread-safe dispatcher for frame observers.
 *
 * Manages a list of observers and dispatches FrameView to each.
 * Exceptions from individual observers are caught and logged to prevent
 * one failing observer from affecting others or the critical recording path.
 */
class FrameDispatcher {
public:
    FrameDispatcher() = default;
    ~FrameDispatcher() = default;

    // Non-copyable
    FrameDispatcher(const FrameDispatcher&) = delete;
    FrameDispatcher& operator=(const FrameDispatcher&) = delete;

    /**
     * @brief Attach an observer to receive frame notifications.
     * @param observer Shared pointer to the observer. Stored as weak_ptr internally.
     */
    void attach(std::shared_ptr<IFrameObserver> observer);

    /**
     * @brief Detach an observer.
     * @param observer The observer to remove.
     */
    void detach(std::shared_ptr<IFrameObserver> observer);

    /**
     * @brief Dispatch a frame to all attached observers.
     *
     * This method is non-blocking and best-effort. If an observer throws,
     * the exception is caught and logged, but other observers continue to receive.
     *
     * @param frame The frame view to dispatch.
     */
    void dispatch(const FrameView& frame);

    /**
     * @brief Get the current number of attached observers.
     */
    size_t observer_count() const;

private:
    mutable std::mutex mutex_;
    std::vector<std::weak_ptr<IFrameObserver>> observers_;
};

} // namespace micecam

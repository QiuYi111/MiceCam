#pragma once

#include "types.h"

namespace micecam {

/**
 * @brief Interface for frame observers.
 * 
 * Observers are notified whenever a new frame is available from the camera.
 * This is the "best-effort" path - observers may miss frames if they are slow.
 * 
 * @warning Implementations MUST return within <1ms. Long-running operations
 * (e.g., deep learning inference) must copy data internally and dispatch
 * to a separate thread.
 */
class IFrameObserver {
public:
    virtual ~IFrameObserver() = default;

    /**
     * @brief Called when a new frame is available.
     * @param frame Read-only view of the frame data. Valid only during this call.
     */
    virtual void on_frame(const FrameView& frame) = 0;
};

} // namespace micecam

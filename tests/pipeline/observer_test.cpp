#include "infrastructure/frame_dispatcher.h"
#include "micecam/observer.h"
#include "micecam/types.h"
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <chrono>

namespace micecam {

// Test observer that counts received frames
class CountingObserver : public IFrameObserver {
public:
    void on_frame(const FrameView& frame) override {
        ++frame_count_;
        last_sequence_id_ = frame.sequence_id;
    }

    std::atomic<int> frame_count_{0};
    std::atomic<uint64_t> last_sequence_id_{0};
};

// Test observer that throws exceptions
class ThrowingObserver : public IFrameObserver {
public:
    void on_frame(const FrameView& /*frame*/) override {
        throw std::runtime_error("Intentional test exception");
    }
};

class FrameDispatcherTest : public ::testing::Test {
protected:
    FrameDispatcher dispatcher_;

    FrameView make_test_frame(uint64_t seq) {
        FrameView view;
        view.data = nullptr;
        view.size = 0;
        view.sequence_id = seq;
        view.timestamp = 0.0;
        view.format = PixelFormat::MJPEG;
        view.width = 1920;
        view.height = 1080;
        view.metadata_json = nullptr;
        return view;
    }
};

TEST_F(FrameDispatcherTest, AttachAndDispatch) {
    auto observer = std::make_shared<CountingObserver>();

    dispatcher_.attach(observer);
    EXPECT_EQ(dispatcher_.observer_count(), 1);

    dispatcher_.dispatch(make_test_frame(1));
    dispatcher_.dispatch(make_test_frame(2));
    dispatcher_.dispatch(make_test_frame(3));

    EXPECT_EQ(observer->frame_count_.load(), 3);
    EXPECT_EQ(observer->last_sequence_id_.load(), 3);
}

TEST_F(FrameDispatcherTest, DetachStopsNotifications) {
    auto observer = std::make_shared<CountingObserver>();

    dispatcher_.attach(observer);
    dispatcher_.dispatch(make_test_frame(1));
    EXPECT_EQ(observer->frame_count_.load(), 1);

    dispatcher_.detach(observer);
    EXPECT_EQ(dispatcher_.observer_count(), 0);

    dispatcher_.dispatch(make_test_frame(2));
    EXPECT_EQ(observer->frame_count_.load(), 1); // Still 1, not notified
}

TEST_F(FrameDispatcherTest, MultipleObservers) {
    auto observer1 = std::make_shared<CountingObserver>();
    auto observer2 = std::make_shared<CountingObserver>();
    auto observer3 = std::make_shared<CountingObserver>();

    dispatcher_.attach(observer1);
    dispatcher_.attach(observer2);
    dispatcher_.attach(observer3);

    EXPECT_EQ(dispatcher_.observer_count(), 3);

    dispatcher_.dispatch(make_test_frame(42));

    EXPECT_EQ(observer1->frame_count_.load(), 1);
    EXPECT_EQ(observer2->frame_count_.load(), 1);
    EXPECT_EQ(observer3->frame_count_.load(), 1);
}

TEST_F(FrameDispatcherTest, WeakPtrCleanup) {
    auto observer = std::make_shared<CountingObserver>();

    dispatcher_.attach(observer);
    EXPECT_EQ(dispatcher_.observer_count(), 1);

    // Destroy the observer
    observer.reset();

    // Dispatch should clean up expired weak_ptr
    dispatcher_.dispatch(make_test_frame(1));
    EXPECT_EQ(dispatcher_.observer_count(), 0);
}

TEST_F(FrameDispatcherTest, ExceptionIsolation) {
    auto throwing = std::make_shared<ThrowingObserver>();
    auto counting = std::make_shared<CountingObserver>();

    dispatcher_.attach(throwing);
    dispatcher_.attach(counting);

    // Dispatch should not throw even though one observer throws
    EXPECT_NO_THROW(dispatcher_.dispatch(make_test_frame(1)));

    // The counting observer should still receive the frame
    EXPECT_EQ(counting->frame_count_.load(), 1);
}

TEST_F(FrameDispatcherTest, ThreadSafety) {
    auto observer = std::make_shared<CountingObserver>();
    dispatcher_.attach(observer);

    std::atomic<bool> running{true};

    // Producer thread
    std::thread producer([this, &running]() {
        uint64_t seq = 0;
        while (running.load()) {
            dispatcher_.dispatch(make_test_frame(seq++));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    running.store(false);
    producer.join();

    EXPECT_GT(observer->frame_count_.load(), 0);
}

} // namespace micecam

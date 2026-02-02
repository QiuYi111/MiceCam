#include "micecam/core/ring_buffer.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace micecam {

class RingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<RingBuffer>(3);  // Small buffer for testing
    }

    std::unique_ptr<RingBuffer> buffer;
};

TEST_F(RingBufferTest, CapacityCheck) {
    EXPECT_EQ(buffer->capacity(), 3);
    EXPECT_TRUE(buffer->empty());
    EXPECT_FALSE(buffer->full());
}

TEST_F(RingBufferTest, PushPop) {
    auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1, 2, 3});
    Frame f1(1, std::move(data));

    buffer->push(std::move(f1));

    EXPECT_EQ(buffer->size(), 1);
    EXPECT_FALSE(buffer->empty());

    auto f2 = buffer->pop();
    EXPECT_EQ(f2.sequence_id, 1);
    EXPECT_EQ(f2.size(), 3);
    EXPECT_TRUE(buffer->empty());
}

TEST_F(RingBufferTest, TryPushSuccess) {
    auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1});
    Frame f(1, std::move(data));

    EXPECT_TRUE(buffer->try_push(std::move(f)));
    EXPECT_EQ(buffer->size(), 1);
}

TEST_F(RingBufferTest, TryPushFull) {
    // Fill buffer
    for (int i = 0; i < 3; ++i) {
        auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1});
        Frame f(i, std::move(data));
        buffer->push(std::move(f));
    }

    auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1});
    Frame f(99, std::move(data));

    EXPECT_FALSE(buffer->try_push(std::move(f)));
    EXPECT_EQ(buffer->size(), 3);
}

TEST_F(RingBufferTest, TryPopEmpty) {
    auto result = buffer->try_pop();
    EXPECT_EQ(result, nullptr);
}

TEST_F(RingBufferTest, FIFOOrder) {
    // Push multiple frames
    for (int i = 0; i < 3; ++i) {
        auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{static_cast<uint8_t>(i)});
        Frame f(i, std::move(data));
        buffer->push(std::move(f));
    }

    // Verify FIFO
    for (int i = 0; i < 3; ++i) {
        auto f = buffer->pop();
        EXPECT_EQ(f.sequence_id, i);
        EXPECT_EQ((*f.data)[0], i);
    }
}

TEST_F(RingBufferTest, BlockingPushOnFull) {
    // Fill buffer
    for (int i = 0; i < 3; ++i) {
        auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1});
        Frame f(i, std::move(data));
        buffer->push(std::move(f));
    }

    // Try to push in another thread - should block
    bool pushed = false;
    std::thread producer([&]() {
        auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1});
        Frame f(99, std::move(data));
        buffer->push(std::move(f));
        pushed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(pushed);  // Still blocked

    buffer->pop();  // Make space
    producer.join();  // Should complete now
    EXPECT_TRUE(pushed);
}

TEST_F(RingBufferTest, BlockingPopOnEmpty) {
    bool popped = false;
    std::thread consumer([&]() {
        buffer->pop();
        popped = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(popped);  // Still blocked

    auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1});
    Frame f(1, std::move(data));
    buffer->push(std::move(f));

    consumer.join();
    EXPECT_TRUE(popped);
}

TEST_F(RingBufferTest, ConcurrentProducerConsumer) {
    const int num_items = 1000;
    std::atomic<int> consumed{0};
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // 2 producers
    for (int p = 0; p < 2; ++p) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < num_items / 2; ++i) {
                auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1});
                Frame f(p * 1000 + i, std::move(data));
                buffer->push(std::move(f));
            }
        });
    }

    // 2 consumers
    for (int c = 0; c < 2; ++c) {
        consumers.emplace_back([&]() {
            for (int i = 0; i < num_items / 2; ++i) {
                buffer->pop();
                consumed.fetch_add(1);
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    EXPECT_EQ(consumed.load(), num_items);
    EXPECT_TRUE(buffer->empty());
}

}  // namespace micecam

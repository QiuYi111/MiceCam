#include "domain/frame.h"
#include <gtest/gtest.h>

namespace micecam {

TEST(FrameTest, DefaultConstruction) {
    Frame f;
    EXPECT_EQ(f.sequence_id, 0);
    EXPECT_EQ(f.size(), 0);
    EXPECT_FALSE(f.data);  // nullptr
}

TEST(FrameTest, ParameterizedConstruction) {
    auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1, 2, 3, 4});
    const uint64_t seq = 42;

    Frame f(seq, std::move(data));

    EXPECT_EQ(f.sequence_id, 42);
    EXPECT_EQ(f.size(), 4);
    EXPECT_TRUE(f.data);
    EXPECT_EQ((*f.data)[0], 1);
    EXPECT_EQ((*f.data)[3], 4);
}

TEST(FrameTest, NonCopyable) {
    auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1, 2, 3});
    Frame f1(1, std::move(data));

    // Frame f2 = f1;  // Should not compile - deleted
    // Frame f3(f1);   // Should not compile - deleted

    SUCCEED();
}

TEST(FrameTest, Moveable) {
    auto data = std::make_unique<std::vector<uint8_t>>(std::vector<uint8_t>{1, 2, 3});
    Frame f1(1, std::move(data));

    Frame f2 = std::move(f1);
    EXPECT_EQ(f2.sequence_id, 1);
    EXPECT_EQ(f2.size(), 3);
    EXPECT_FALSE(f1.data);  // Moved from
}

}  // namespace micecam

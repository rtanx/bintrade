#include "core/platform/clock.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace bintrade::test {

TEST(PlatformClockTest, ReturnsPositiveValue) {
    std::int64_t t = bintrade::platform::monotonic_now_ns();
    EXPECT_GT(t, 0);
}

TEST(PlatformClockTest, IsMonotonicallyNonDecreasing) {
    std::int64_t t1 = bintrade::platform::monotonic_now_ns();
    std::int64_t t2 = bintrade::platform::monotonic_now_ns();
    EXPECT_GE(t2, t1);
}

TEST(PlatformClockTest, ElapsedMatchesSleepWithSlack) {
    std::int64_t before = bintrade::platform::monotonic_now_ns();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::int64_t after = bintrade::platform::monotonic_now_ns();
    std::int64_t elapsed = after - before;

    // Lower bound: at least 0.5 ms -- generous to handle loaded CI runners.
    EXPECT_GE(elapsed, 500'000LL);
    // Upper bound: less than 500 ms -- a sanity check against a broken clock.
    EXPECT_LT(elapsed, 500'000'000LL);
}

}  // namespace bintrade::test

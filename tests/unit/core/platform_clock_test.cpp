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

// -----------------------------------------------------------------------
// ticks_to_ns helper -- platform-agnostic conversion math.
// Pre-Phase-5 the Windows path open-coded `(ticks * 1e9) / freq`, which
// overflows int64 after about one hour of uptime at a 10 MHz QPC
// frequency.  These tests pin the helper's behaviour for the values
// that triggered the original CI failure.
// -----------------------------------------------------------------------

TEST(PlatformClockTickConversion, ZeroTicksIsZero) {
    EXPECT_EQ(bintrade::platform::ticks_to_ns(0, 10'000'000), 0);
}

TEST(PlatformClockTickConversion, OneSecondAtTenMhz) {
    EXPECT_EQ(bintrade::platform::ticks_to_ns(10'000'000, 10'000'000), 1'000'000'000);
}

TEST(PlatformClockTickConversion, SubSecondRemainderPreserved) {
    // 7'500'000 ticks at 10 MHz = 0.75 s = 750'000'000 ns.
    EXPECT_EQ(bintrade::platform::ticks_to_ns(7'500'000, 10'000'000), 750'000'000);
}

TEST(PlatformClockTickConversion, NoOverflowAtTenHoursUptime) {
    // 10 hours at 10 MHz = 3.6e11 ticks.  `ticks * 1'000'000'000`
    // would be 3.6e20, well past int64_t max (~9.22e18).  The split
    // implementation must still return a positive, exact result.
    constexpr std::int64_t ten_hours_ticks = std::int64_t{10} * 3600 * 10'000'000;
    constexpr std::int64_t expected_ns = std::int64_t{10} * 3600 * 1'000'000'000;
    EXPECT_EQ(bintrade::platform::ticks_to_ns(ten_hours_ticks, 10'000'000), expected_ns);
}

TEST(PlatformClockTickConversion, NoOverflowAtTenYearsUptime) {
    // 10 years at 10 MHz, well past any realistic CI runner lifespan.
    // Still must yield a positive, monotonic, exact-second value.
    constexpr std::int64_t seconds_per_year = std::int64_t{365} * 24 * 3600;
    constexpr std::int64_t ten_years_ticks = 10 * seconds_per_year * 10'000'000;
    constexpr std::int64_t expected_ns = 10 * seconds_per_year * 1'000'000'000;
    EXPECT_EQ(bintrade::platform::ticks_to_ns(ten_years_ticks, 10'000'000), expected_ns);
    EXPECT_GT(bintrade::platform::ticks_to_ns(ten_years_ticks, 10'000'000), 0);
}

}  // namespace bintrade::test

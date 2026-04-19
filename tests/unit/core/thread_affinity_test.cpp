#include "core/platform/thread_affinity.hpp"

#include <gtest/gtest.h>

namespace bintrade::test {

// core_id < 0 must always be a no-op that returns true on every platform.
TEST(ThreadAffinityTest, NoopWhenCoreIdNegative) {
    EXPECT_TRUE(bintrade::platform::pin_thread_to_core(-1));
    EXPECT_TRUE(bintrade::platform::pin_thread_to_core(-100));
}

// core_id = 0 must not throw (noexcept contract) and must return a bool.
// The actual success/failure is environment-dependent: CI containers may
// deny affinity changes, and macOS always returns true (advisory hint).
TEST(ThreadAffinityTest, CoreZeroDoesNotThrow) {
    bool result = false;
    EXPECT_NO_THROW(result = bintrade::platform::pin_thread_to_core(0));
    // Silence "unused variable" warning; we only care about no-throw.
    (void)result;
}

// A wildly out-of-range core id must not crash or throw.
// On Linux it returns false (EINVAL), on macOS true (advisory tag),
// on Windows false (core_id >= 64 guard).
TEST(ThreadAffinityTest, InvalidCoreIdDoesNotCrash) {
    bool result = false;
    EXPECT_NO_THROW(result = bintrade::platform::pin_thread_to_core(9'999));
    (void)result;
}

}  // namespace bintrade::test

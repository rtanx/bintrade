#pragma once

#include <cstdint>

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#else
#    include <time.h>  // NOLINT(modernize-deprecated-headers): POSIX clock_gettime
#endif

namespace bintrade::platform {

// Returns the current value of a monotonic clock in nanoseconds.
//
// The epoch is arbitrary (typically system boot or process start) and must
// not be used for wall-clock time. Use only for measuring elapsed intervals.
//
// Platform implementation:
//   Linux/macOS: clock_gettime(CLOCK_MONOTONIC)
//   Windows    : QueryPerformanceCounter with a process-lifetime cached
//                frequency to amortise the QueryPerformanceFrequency cost.
//
// Never throws. Never allocates.
[[nodiscard]] inline std::int64_t monotonic_now_ns() noexcept {
#if defined(_WIN32)
    // Cache the QPC frequency once. Guaranteed to be non-zero on any
    // hardware that supports QPC (all x64 Windows systems since Vista).
    static const LARGE_INTEGER kFreq = []() noexcept -> LARGE_INTEGER {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        return f;
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    // Multiply before dividing to preserve sub-nanosecond resolution.
    // kFreq.QuadPart is ~10^7 on modern hardware; the intermediate product
    // fits in int64_t for system uptimes up to ~292 years.
    return (counter.QuadPart * std::int64_t{1'000'000'000}) / kFreq.QuadPart;
#else
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (static_cast<std::int64_t>(ts.tv_sec) * std::int64_t{1'000'000'000}) + static_cast<std::int64_t>(ts.tv_nsec);
#endif
}

}  // namespace bintrade::platform

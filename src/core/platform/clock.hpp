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

// Convert a (ticks, ticks-per-second) pair into nanoseconds without
// overflowing int64 for the realistic uptime range of a CI runner.
//
// The naive expression `(ticks * 1e9) / freq` overflows int64 after
// roughly one hour of uptime at the typical 10 MHz QPC frequency on
// Windows, which is what bit us in CI.  Splitting the computation into
// whole seconds plus a sub-second remainder keeps every intermediate
// product within int64 for uptimes past a century.
//
// Precondition: freq > 0.  Negative ticks are accepted but never
// produced by QPC.
[[nodiscard]] constexpr std::int64_t ticks_to_ns(std::int64_t ticks, std::int64_t freq) noexcept {
    const std::int64_t whole_seconds = ticks / freq;
    const std::int64_t remainder_ticks = ticks % freq;
    return (whole_seconds * std::int64_t{1'000'000'000}) + ((remainder_ticks * std::int64_t{1'000'000'000}) / freq);
}

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
    static const std::int64_t k_freq = []() noexcept -> std::int64_t {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        return f.QuadPart;
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return ticks_to_ns(counter.QuadPart, k_freq);
#else
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (static_cast<std::int64_t>(ts.tv_sec) * std::int64_t{1'000'000'000}) + static_cast<std::int64_t>(ts.tv_nsec);
#endif
}

}  // namespace bintrade::platform

#pragma once

namespace bintrade::platform {

// Pin the calling thread to the given logical CPU core.
//
// Returns true on success or when core_id < 0 (no-op).
// Returns false if the OS call fails (e.g. invalid core_id, permission denied).
// The return value is informational; callers that want best-effort pinning
// (such as the WebSocket I/O thread) may safely discard it.
// Never throws.
//
// Platform behaviour:
//   Linux  : pthread_setaffinity_np + cpu_set_t (hard pinning)
//   macOS  : thread_policy_set(THREAD_AFFINITY_POLICY) -- advisory hint only;
//             the kernel may schedule the thread on a different core.
//   Windows: SetThreadAffinityMask(GetCurrentThread(), 1ULL << core_id);
//             capped at core 63 (single processor group limit).
//   Other  : no-op, returns true.
bool pin_thread_to_core(int core_id) noexcept;

}  // namespace bintrade::platform

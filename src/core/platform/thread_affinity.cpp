#include "thread_affinity.hpp"

#ifdef __linux__
#    include <pthread.h>
#    include <sched.h>
#    include <cstddef>
#elif defined(__APPLE__)
#    include <mach/mach.h>
#    include <mach/thread_policy.h>
#elif defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif

namespace bintrade::platform {

bool pin_thread_to_core(int core_id) noexcept {
    if (core_id < 0) {
        return true;
    }

#ifdef __linux__
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(static_cast<std::size_t>(core_id), &cpu_set);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) == 0;

#elif defined(__APPLE__)
    // mach_thread_self() returns a send right that the caller owns; must be
    // deallocated after use to avoid Mach port leaks.
    mach_port_t thread_port = mach_thread_self();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): Mach API requires thread_policy_t (integer_t*)
    thread_affinity_policy_data_t policy{static_cast<integer_t>(core_id)};
    kern_return_t result =
        thread_policy_set(thread_port, THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(&policy), THREAD_AFFINITY_POLICY_COUNT);
    mach_port_deallocate(mach_task_self(), thread_port);
    return result == KERN_SUCCESS;

#elif defined(_WIN32)
    // SetThreadAffinityMask is limited to the first 64 logical processors
    // within a processor group. Return false for out-of-range core ids.
    if (core_id >= 64) {
        return false;
    }
    DWORD_PTR mask = DWORD_PTR{1} << static_cast<unsigned>(core_id);
    return SetThreadAffinityMask(GetCurrentThread(), mask) != 0;

#else
    // Unsupported platform: treat as a successful no-op so callers do not
    // need to handle a spurious false return on unknown systems.
    return true;
#endif
}

}  // namespace bintrade::platform

#pragma once

// Lock-free single-producer single-consumer bounded queue.
//
// Capacity is a compile-time power-of-two. Uses acquire/release memory
// ordering only -- never seq_cst. Head and tail counters are padded to
// separate cache lines to prevent false sharing.

#include <atomic>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace bintrade {

// Hardcoded to 64 bytes (L1 line size on x86_64 and ARM). We deliberately do
// not use std::hardware_destructive_interference_size because GCC warns
// (-Winterference-size) that its value can shift with -mtune flags and is
// unsafe across translation units that may be compiled differently. Boost
// Lockfree, Folly, and most HFT libraries hardcode the same value for the
// same reason.
inline constexpr std::size_t k_cache_line_size = 64;

/// Lock-free bounded SPSC (single-producer single-consumer) ring buffer.
///
/// @tparam T        Element type. Must be nothrow move constructible.
/// @tparam Capacity Fixed queue capacity. Must be a power of two.
///
/// The producer thread calls try_push(); the consumer thread calls try_pop().
/// No other synchronization is required between the two threads.
///
/// Example:
///   SpscQueue<std::string, 1024> q;
///   // Producer thread:
///   q.try_push("hello");
///   // Consumer thread:
///   auto msg = q.try_pop();  // optional<string>
template <typename T, std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0, "SpscQueue capacity must be a power of two");
    static_assert(std::is_nothrow_move_constructible_v<T>, "SpscQueue element type must be nothrow move constructible");

public:
    SpscQueue() noexcept = default;

    ~SpscQueue() noexcept {
        // Drain remaining elements so their destructors run.
        while (try_pop().has_value()) {}
    }

    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;
    SpscQueue(SpscQueue&&) = delete;
    SpscQueue& operator=(SpscQueue&&) = delete;

    /// Enqueue an element (lvalue). Returns false if the queue is full.
    /// The source is not modified when the queue is full.
    /// Must be called from the producer thread only.
    [[nodiscard]] bool try_push(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);

        if (head - tail >= Capacity) {
            return false;  // Full.
        }

        buffer_[head & k_mask] = value;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    /// Enqueue an element (rvalue). Returns false if the queue is full.
    /// The source is not modified when the queue is full.
    /// Must be called from the producer thread only.
    [[nodiscard]] bool try_push(T&& value) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);

        if (head - tail >= Capacity) {
            return false;  // Full.
        }

        buffer_[head & k_mask] = std::move(value);
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    /// Dequeue an element. Returns std::nullopt if the queue is empty.
    /// Must be called from the consumer thread only.
    [[nodiscard]] std::optional<T> try_pop() noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);

        if (tail == head) {
            return std::nullopt;  // Empty.
        }

        std::optional<T> result{std::move(buffer_[tail & k_mask])};
        tail_.store(tail + 1, std::memory_order_release);
        return result;
    }

    /// Approximate number of elements in the queue.
    /// The result may be stale by the time the caller reads it.
    [[nodiscard]] std::size_t size_approx() const noexcept {
        const auto head = head_.load(std::memory_order_acquire);
        const auto tail = tail_.load(std::memory_order_acquire);
        return head - tail;
    }

    /// Returns true if the queue appears empty. May be stale.
    [[nodiscard]] bool empty() const noexcept { return size_approx() == 0; }

    /// Returns the fixed capacity of the queue.
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    static constexpr std::size_t k_mask = Capacity - 1;

    // Head (write position) and tail (read position) on separate cache lines
    // to prevent false sharing between producer and consumer.
    alignas(k_cache_line_size) std::atomic<std::size_t> head_{0};
    alignas(k_cache_line_size) std::atomic<std::size_t> tail_{0};

    // Element storage. No per-slot padding: for large T (e.g. std::string)
    // padding each slot to 64 bytes would waste memory with bounded capacity.
    // Producer and consumer access different indices, so false sharing between
    // adjacent slots is not a concern in the SPSC pattern.
    alignas(k_cache_line_size) T buffer_[Capacity]{};
};

}  // namespace bintrade

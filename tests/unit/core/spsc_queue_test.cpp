#include <bintrade/core/spsc_queue.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

// -----------------------------------------------------------------------
// Basic operations
// -----------------------------------------------------------------------

TEST(SpscQueueTest, SinglePushPop) {
    bintrade::SpscQueue<int, 4> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size_approx(), 0U);

    ASSERT_TRUE(q.try_push(42));
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.size_approx(), 1U);

    auto val = q.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42);
    EXPECT_TRUE(q.empty());
}

TEST(SpscQueueTest, FillAndDrainFifo) {
    constexpr std::size_t cap = 8;
    bintrade::SpscQueue<int, cap> q;

    for (int i = 0; i < static_cast<int>(cap); ++i) {
        ASSERT_TRUE(q.try_push(i));
    }
    EXPECT_EQ(q.size_approx(), cap);

    for (int i = 0; i < static_cast<int>(cap); ++i) {
        auto val = q.try_pop();
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, i) << "FIFO order violated at index " << i;
    }
    EXPECT_TRUE(q.empty());
}

TEST(SpscQueueTest, PushWhenFullReturnsFalse) {
    bintrade::SpscQueue<int, 2> q;
    ASSERT_TRUE(q.try_push(1));
    ASSERT_TRUE(q.try_push(2));
    EXPECT_FALSE(q.try_push(3));  // Full.
    EXPECT_EQ(q.size_approx(), 2U);
}

TEST(SpscQueueTest, PopWhenEmptyReturnsNullopt) {
    bintrade::SpscQueue<int, 4> q;
    auto val = q.try_pop();
    EXPECT_FALSE(val.has_value());
}

TEST(SpscQueueTest, SizeApprox) {
    bintrade::SpscQueue<int, 16> q;
    for (int i = 0; i < 7; ++i) {
        ASSERT_TRUE(q.try_push(i));
    }
    EXPECT_EQ(q.size_approx(), 7U);

    (void)q.try_pop();
    (void)q.try_pop();
    EXPECT_EQ(q.size_approx(), 5U);
}

TEST(SpscQueueTest, Capacity) {
    bintrade::SpscQueue<int, 64> q;
    EXPECT_EQ(q.capacity(), 64U);
}

// -----------------------------------------------------------------------
// Wrap-around
// -----------------------------------------------------------------------

TEST(SpscQueueTest, WrapAround) {
    // Push and pop more than Capacity times to exercise bitmask wrap-around.
    constexpr std::size_t cap = 4;
    bintrade::SpscQueue<int, cap> q;

    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < static_cast<int>(cap); ++i) {
            ASSERT_TRUE(q.try_push((round * 100) + i));
        }
        for (int i = 0; i < static_cast<int>(cap); ++i) {
            auto val = q.try_pop();
            ASSERT_TRUE(val.has_value());
            EXPECT_EQ(*val, (round * 100) + i);
        }
        EXPECT_TRUE(q.empty());
    }
}

// -----------------------------------------------------------------------
// Move-only types
// -----------------------------------------------------------------------

TEST(SpscQueueTest, MoveOnlyType) {
    bintrade::SpscQueue<std::unique_ptr<int>, 4> q;

    ASSERT_TRUE(q.try_push(std::make_unique<int>(99)));

    auto val = q.try_pop();
    ASSERT_TRUE(val.has_value());
    ASSERT_NE(*val, nullptr);
    EXPECT_EQ(**val, 99);
}

// -----------------------------------------------------------------------
// String elements (typical hot-path use case)
// -----------------------------------------------------------------------

TEST(SpscQueueTest, StringElements) {
    bintrade::SpscQueue<std::string, 4> q;

    ASSERT_TRUE(q.try_push("hello"));
    ASSERT_TRUE(q.try_push("world"));

    auto v1 = q.try_pop();
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, "hello");

    auto v2 = q.try_pop();
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, "world");
}

// -----------------------------------------------------------------------
// Concurrent producer/consumer
// -----------------------------------------------------------------------

TEST(SpscQueueTest, ConcurrentProducerConsumer) {
    constexpr std::size_t cap = 1'024;
    constexpr int num_items = 100'000;

    bintrade::SpscQueue<int, cap> q;
    std::vector<int> received;
    received.reserve(num_items);

    std::atomic<bool> producer_done{false};

    // Producer thread.
    std::thread producer([&] {
        for (int i = 0; i < num_items; ++i) {
            while (!q.try_push(i)) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer thread.
    std::thread consumer([&] {
        while (true) {
            if (auto val = q.try_pop()) {
                received.push_back(*val);
                if (static_cast<int>(received.size()) == num_items) {
                    break;
                }
            } else if (producer_done.load(std::memory_order_acquire)) {
                // Drain remaining after producer signals done.
                while (auto remaining = q.try_pop()) {
                    received.push_back(*remaining);
                }
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    // Verify completeness and order.
    ASSERT_EQ(static_cast<int>(received.size()), num_items);
    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(received[static_cast<std::size_t>(i)], i) << "Out-of-order at index " << i;
    }
}

TEST(SpscQueueTest, ConcurrentStringProducerConsumer) {
    constexpr std::size_t cap = 256;
    constexpr int num_items = 10'000;

    bintrade::SpscQueue<std::string, cap> q;
    std::vector<std::string> received;
    received.reserve(num_items);

    std::atomic<bool> producer_done{false};

    std::thread producer([&] {
        for (int i = 0; i < num_items; ++i) {
            auto msg = "msg_" + std::to_string(i);
            while (!q.try_push(std::move(msg))) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        while (true) {
            if (auto val = q.try_pop()) {
                received.push_back(std::move(*val));
                if (static_cast<int>(received.size()) == num_items) {
                    break;
                }
            } else if (producer_done.load(std::memory_order_acquire)) {
                while (auto remaining = q.try_pop()) {
                    received.push_back(std::move(*remaining));
                }
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(static_cast<int>(received.size()), num_items);
    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(received[static_cast<std::size_t>(i)], "msg_" + std::to_string(i));
    }
}

// -----------------------------------------------------------------------
// Destructor drains remaining elements
// -----------------------------------------------------------------------

TEST(SpscQueueTest, DestructorDrainsElements) {
    static int alive_count = 0;

    struct Tracked {
        Tracked() noexcept { ++alive_count; }
        Tracked(const Tracked& /*other*/) noexcept { ++alive_count; }
        Tracked(Tracked&& /*other*/) noexcept { ++alive_count; }
        ~Tracked() noexcept { --alive_count; }
        Tracked& operator=(const Tracked&) noexcept = default;
        Tracked& operator=(Tracked&&) noexcept = default;
    };

    alive_count = 0;
    {
        // The default-constructed buffer_ elements are alive.
        bintrade::SpscQueue<Tracked, 4> q;
        int baseline = alive_count;

        Tracked t;
        (void)q.try_push(std::move(t));
        (void)q.try_push(Tracked{});
        EXPECT_GT(alive_count, baseline);
    }
    // After destruction, all managed elements should be cleaned up.
    EXPECT_EQ(alive_count, 0);
}

}  // namespace

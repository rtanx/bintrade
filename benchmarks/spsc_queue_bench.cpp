// SPSC queue throughput benchmarks.
//
// BM_SpscPushPop: single-threaded push+pop throughput.
// BM_SpscProducerConsumer: two-thread producer/consumer throughput.

#include <bintrade/core/spsc_queue.hpp>

#include <benchmark/benchmark.h>

#include <atomic>
#include <string>
#include <thread>

namespace {

// ---------------------------------------------------------------------------
// Single-threaded push + pop cycle.
// Measures raw per-operation overhead without contention.
// ---------------------------------------------------------------------------
void bm_spsc_push_pop_int(benchmark::State& state) {
    bintrade::SpscQueue<int, 1024> q;
    int val = 0;
    for (auto _ : state) {
        (void)q.try_push(val);
        auto result = q.try_pop();
        benchmark::DoNotOptimize(result);
        ++val;
    }
}
BENCHMARK(bm_spsc_push_pop_int);

void bm_spsc_push_pop_string(benchmark::State& state) {
    bintrade::SpscQueue<std::string, 1024> q;
    // Typical short JSON message length (SSO eligible on most implementations).
    const std::string msg(64, 'x');
    for (auto _ : state) {
        (void)q.try_push(std::string(msg));
        auto result = q.try_pop();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(bm_spsc_push_pop_string);

// ---------------------------------------------------------------------------
// Two-thread producer/consumer throughput.
// Reports ops/sec of sustained push+pop across threads.
// ---------------------------------------------------------------------------
void bm_spsc_producer_consumer(benchmark::State& state) {
    constexpr std::size_t cap = 4096;
    bintrade::SpscQueue<int, cap> q;

    for (auto _ : state) {
        state.PauseTiming();
        constexpr int num_items = 100'000;
        std::atomic<bool> done{false};
        int consumed = 0;

        std::thread consumer([&] {
            while (true) {
                if (auto val = q.try_pop()) {
                    benchmark::DoNotOptimize(*val);
                    ++consumed;
                    if (consumed == num_items) {
                        break;
                    }
                } else if (done.load(std::memory_order_acquire)) {
                    while (auto remaining = q.try_pop()) {
                        benchmark::DoNotOptimize(*remaining);
                        ++consumed;
                    }
                    break;
                } else {
                    std::this_thread::yield();
                }
            }
        });

        state.ResumeTiming();

        for (int i = 0; i < num_items; ++i) {
            while (!q.try_push(i)) {
                std::this_thread::yield();
            }
        }
        done.store(true, std::memory_order_release);

        consumer.join();
        state.SetItemsProcessed(num_items);
    }
}
BENCHMARK(bm_spsc_producer_consumer)->Unit(benchmark::kMillisecond);

}  // namespace

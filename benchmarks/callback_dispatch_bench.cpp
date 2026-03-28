// Callback dispatch benchmarks.
//
// Compares the overhead of three dispatch strategies:
//   1. std::function (indirect call through type-erased wrapper)
//   2. CRTP / direct handler method call (zero-overhead, devirtualized)
//   3. FunctionalMarketHandler (std::function behind on_* adapter)
//
// These benchmarks isolate the *dispatch* cost from JSON parsing.
// The trade object is pre-built so we measure only the callback path.

#include <bintrade/models/trade.hpp>
#include <bintrade/ws/detail/ws_parse.hpp>
#include <bintrade/ws/handler_traits.hpp>

#include <benchmark/benchmark.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace {

// Pre-parsed trade for dispatch-only benchmarks.
bintrade::models::Trade make_sample_trade() {
    constexpr std::string_view json = R"({
      "e":"trade","E":1704067200123,"s":"BTCUSDT","t":100000001,
      "p":"42000.50","q":"0.123","T":1704067200100,"m":false
    })";
    auto j = nlohmann::json::parse(json);
    return bintrade::ws::detail::parse_trade_stream(j);
}

// ---------------------------------------------------------------------------
// 1. std::function dispatch
// ---------------------------------------------------------------------------
void bm_std_function_dispatch(benchmark::State& state) {
    auto trade = make_sample_trade();
    int count = 0;
    std::function<void(const bintrade::models::Trade&)> callback =
        [&count](const bintrade::models::Trade& /*t*/) {
            ++count;
            benchmark::ClobberMemory();
        };

    for (auto _ : state) {
        callback(trade);
    }
    benchmark::DoNotOptimize(count);
}
BENCHMARK(bm_std_function_dispatch);

// ---------------------------------------------------------------------------
// 2. CRTP-style direct handler call
// ---------------------------------------------------------------------------
struct DirectHandler {
    int count = 0;
    void on_trade(const bintrade::models::Trade& /*t*/) noexcept {
        ++count;
        benchmark::ClobberMemory();
    }
};

template <typename Handler>
void dispatch_crtp(Handler& handler, const bintrade::models::Trade& trade) {
    if constexpr (requires { handler.on_trade(trade); }) {
        handler.on_trade(trade);
    }
}

void bm_crtp_dispatch(benchmark::State& state) {
    auto trade = make_sample_trade();
    DirectHandler handler;

    for (auto _ : state) {
        dispatch_crtp(handler, trade);
    }
    benchmark::DoNotOptimize(handler.count);
}
BENCHMARK(bm_crtp_dispatch);

// ---------------------------------------------------------------------------
// 3. FunctionalMarketHandler dispatch (std::function behind adapter)
// ---------------------------------------------------------------------------
void bm_functional_handler_dispatch(benchmark::State& state) {
    auto trade = make_sample_trade();
    int count = 0;
    bintrade::ws::FunctionalMarketHandler handler;
    handler.on_trade_fn = [&count](const bintrade::models::Trade& /*t*/) {
        ++count;
        benchmark::ClobberMemory();
    };

    for (auto _ : state) {
        handler.on_trade(trade);
    }
    benchmark::DoNotOptimize(count);
}
BENCHMARK(bm_functional_handler_dispatch);

// ---------------------------------------------------------------------------
// 4. End-to-end: JSON parse + std::function dispatch
// ---------------------------------------------------------------------------
void bm_end_to_end_std_function(benchmark::State& state) {
    constexpr std::string_view raw = R"({
      "e":"trade","E":1704067200123,"s":"BTCUSDT","t":100000001,
      "p":"42000.50","q":"0.123","T":1704067200100,"m":false
    })";
    const std::string payload(raw);
    int count = 0;
    std::function<void(const bintrade::models::Trade&)> callback =
        [&count](const bintrade::models::Trade& /*t*/) {
            ++count;
            benchmark::ClobberMemory();
        };

    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        if (j.value("e", "") == "trade") {
            callback(bintrade::ws::detail::parse_trade_stream(j));
        }
    }
    benchmark::DoNotOptimize(count);
}
BENCHMARK(bm_end_to_end_std_function);

// ---------------------------------------------------------------------------
// 5. End-to-end: JSON parse + CRTP dispatch
// ---------------------------------------------------------------------------
void bm_end_to_end_crtp(benchmark::State& state) {
    constexpr std::string_view raw = R"({
      "e":"trade","E":1704067200123,"s":"BTCUSDT","t":100000001,
      "p":"42000.50","q":"0.123","T":1704067200100,"m":false
    })";
    const std::string payload(raw);
    DirectHandler handler;

    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        if (j.value("e", "") == "trade") {
            dispatch_crtp(handler, bintrade::ws::detail::parse_trade_stream(j));
        }
    }
    benchmark::DoNotOptimize(handler.count);
}
BENCHMARK(bm_end_to_end_crtp);

}  // namespace

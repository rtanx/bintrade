// WebSocket JSON parse benchmarks.
//
// Measures the cost of parsing each Binance WebSocket stream message
// type from raw JSON into the corresponding model struct.  These are
// the operations that Task 8 (DispatchMode::Queued) moves off the I/O
// thread, so their per-message cost directly motivates the queued path.

#include <bintrade/ws/detail/ws_parse.hpp>

#include <benchmark/benchmark.h>

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace {

// ---------------------------------------------------------------------------
// Fixture JSON payloads -- representative Binance messages.
// ---------------------------------------------------------------------------

constexpr std::string_view trade_json = R"({
  "e":"trade","E":1704067200123,"s":"BTCUSDT","t":100000001,
  "p":"42000.50","q":"0.123","b":200000001,"a":200000002,
  "T":1704067200100,"m":false,"M":true
})";

constexpr std::string_view kline_json = R"({
  "e":"kline","E":1704067200123,"s":"BTCUSDT",
  "k":{"t":1704067200000,"T":1704153600000,"s":"BTCUSDT","i":"1h",
       "o":"42000.00","c":"42500.00","h":"43000.00","l":"41000.00",
       "v":"100.5","q":"4221750.00","n":500,"x":false}
})";

constexpr std::string_view ticker_json = R"({
  "e":"24hrTicker","E":1704067200123,"s":"BTCUSDT",
  "p":"1500.00","P":"3.70","o":"40500.00","h":"43000.00",
  "l":"40000.00","c":"42000.50","v":"12345.678",
  "q":"518400000.00","O":1704067200000,"C":1704153600000,"n":987654
})";

constexpr std::string_view depth_json = R"({
  "e":"depthUpdate","E":1704067200123,"s":"BTCUSDT",
  "U":12345670,"u":12345678,
  "b":[["42000.00","1.500"],["41999.00","2.300"],["41998.00","0.800"],
       ["41997.00","1.100"],["41996.00","3.200"]],
  "a":[["42001.00","0.750"],["42002.00","1.200"],["42003.00","0.500"],
       ["42004.00","2.100"],["42005.00","1.800"]]
})";

constexpr std::string_view order_update_json = R"({
  "e":"executionReport","E":1704067200123,"s":"BTCUSDT",
  "S":"BUY","o":"LIMIT","f":"GTC","q":"1.000","p":"42000.00",
  "x":"NEW","X":"NEW","i":123456789,"l":"0.000","z":"0.000",
  "L":"0.00","n":"0.00","N":"BTC","T":1704067200100,"t":0,
  "O":1704067200000,"Z":"0.00"
})";

constexpr std::string_view account_update_json = R"({
  "e":"outboundAccountPosition","E":1704067200123,"u":1704067200100,
  "B":[
    {"a":"BTC","f":"1.50000000","l":"0.50000000"},
    {"a":"USDT","f":"50000.00","l":"10000.00"},
    {"a":"ETH","f":"25.00000000","l":"5.00000000"}
  ]
})";

// ---------------------------------------------------------------------------
// Benchmarks -- each measures nlohmann::json::parse + model conversion.
// ---------------------------------------------------------------------------

void bm_parse_trade_stream(benchmark::State& state) {
    const std::string payload(trade_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        auto trade = bintrade::ws::detail::parse_trade_stream(j);
        benchmark::DoNotOptimize(trade);
    }
}
BENCHMARK(bm_parse_trade_stream);

void bm_parse_kline_stream(benchmark::State& state) {
    const std::string payload(kline_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        auto kline = bintrade::ws::detail::parse_kline_stream(j);
        benchmark::DoNotOptimize(kline);
    }
}
BENCHMARK(bm_parse_kline_stream);

void bm_parse_ticker_stream(benchmark::State& state) {
    const std::string payload(ticker_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        auto ticker = bintrade::ws::detail::parse_ticker_stream(j);
        benchmark::DoNotOptimize(ticker);
    }
}
BENCHMARK(bm_parse_ticker_stream);

void bm_parse_depth_stream(benchmark::State& state) {
    const std::string payload(depth_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        auto book = bintrade::ws::detail::parse_depth_stream(j);
        benchmark::DoNotOptimize(book);
    }
}
BENCHMARK(bm_parse_depth_stream);

void bm_parse_order_update(benchmark::State& state) {
    const std::string payload(order_update_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        auto order = bintrade::ws::detail::parse_order_update(j);
        benchmark::DoNotOptimize(order);
    }
}
BENCHMARK(bm_parse_order_update);

void bm_parse_account_update(benchmark::State& state) {
    const std::string payload(account_update_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        auto account = bintrade::ws::detail::parse_account_update(j);
        benchmark::DoNotOptimize(account);
    }
}
BENCHMARK(bm_parse_account_update);

// ---------------------------------------------------------------------------
// JSON parse only (no model conversion) -- isolates nlohmann overhead.
// ---------------------------------------------------------------------------

void bm_json_parse_only_trade(benchmark::State& state) {
    const std::string payload(trade_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        benchmark::DoNotOptimize(j);
    }
}
BENCHMARK(bm_json_parse_only_trade);

void bm_json_parse_only_depth(benchmark::State& state) {
    const std::string payload(depth_json);
    for (auto _ : state) {
        auto j = nlohmann::json::parse(payload);
        benchmark::DoNotOptimize(j);
    }
}
BENCHMARK(bm_json_parse_only_depth);

}  // namespace

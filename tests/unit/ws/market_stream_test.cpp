// Unit tests for bintrade::ws::MarketStream
//
// Tests focus on the observable behaviour exercisable without a live network
// connection:
//   - Each subscribe_*() method connects to the correct stream path
//   - Matching event messages are dispatched to the registered callback
//   - Non-matching event types are silently discarded
//   - Malformed JSON is silently discarded (no crash, no callback)
//   - Symbol names are normalised to lowercase in the stream path

#include "binance_responses.hpp"

#include <bintrade/core/config.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/ws/market_stream.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <string_view>

namespace {

using namespace bintrade;
using namespace bintrade::ws;
namespace fixtures = bintrade::test::fixtures;

// ---------------------------------------------------------------------------
// TestableMarketStream
//
// Overrides do_connect() to suppress real network I/O and record the path.
// Exposes inject() to pump raw JSON through the registered message callback.
// ---------------------------------------------------------------------------
class TestableMarketStream : public MarketStream {
public:
    using MarketStream::MarketStream;

    std::string last_connected_path;

    // Simulate message arrival from the I/O thread.
    void inject(std::string_view msg) { deliver_message(msg); }

protected:
    void do_connect(std::string_view stream_name) override {
        last_connected_path = std::string(stream_name);
        // No real network I/O.
    }
};

// ---------------------------------------------------------------------------
// subscribe_trades
// ---------------------------------------------------------------------------
TEST(MarketStreamTest, SubscribeTradesConnectsToCorrectPath) {
    TestableMarketStream ms;
    ms.subscribe_trades("btcusdt", [](const models::Trade& /*t*/) {});
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@trade");
}

TEST(MarketStreamTest, SubscribeTradesNormalisesSymbolToLower) {
    TestableMarketStream ms;
    ms.subscribe_trades("BTCUSDT", [](const models::Trade& /*t*/) {});
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@trade");
}

TEST(MarketStreamTest, SubscribeTradesDispatchesTradeCallback) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};
    models::Trade received;

    ms.subscribe_trades("btcusdt", [&](const models::Trade& t) {
        received = t;
        ++call_count;
    });

    ms.inject(fixtures::ws_trade_stream);

    ASSERT_EQ(call_count.load(), 1);
    EXPECT_EQ(received.id, 100'000'001UL);
    EXPECT_NEAR(received.price, 42000.50, 0.001);
}

TEST(MarketStreamTest, SubscribeTradesIgnoresNonTradeEvents) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};

    ms.subscribe_trades("btcusdt", [&](const models::Trade& /*t*/) { ++call_count; });
    // Inject a kline message -- should not trigger the trade callback.
    ms.inject(fixtures::ws_kline_stream);

    EXPECT_EQ(call_count.load(), 0);
}

TEST(MarketStreamTest, SubscribeTradesIgnoresMalformedJson) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};

    ms.subscribe_trades("btcusdt", [&](const models::Trade& /*t*/) { ++call_count; });
    ms.inject("{{{not valid json");

    EXPECT_EQ(call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// subscribe_klines
// ---------------------------------------------------------------------------
TEST(MarketStreamTest, SubscribeKlinesConnectsToCorrectPath) {
    TestableMarketStream ms;
    ms.subscribe_klines("BTCUSDT", "1h", [](const models::Kline& /*k*/) {});
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@kline_1h");
}

TEST(MarketStreamTest, SubscribeKlinesDispatchesKlineCallback) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};
    models::Kline received;

    ms.subscribe_klines("btcusdt", "1h", [&](const models::Kline& k) {
        received = k;
        ++call_count;
    });

    ms.inject(fixtures::ws_kline_stream);

    ASSERT_EQ(call_count.load(), 1);
    EXPECT_NEAR(received.open, 42000.00, 0.001);
    EXPECT_EQ(received.num_trades, 500UL);
}

TEST(MarketStreamTest, SubscribeKlinesIgnoresNonKlineEvents) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};

    ms.subscribe_klines("btcusdt", "1h", [&](const models::Kline& /*k*/) { ++call_count; });
    ms.inject(fixtures::ws_trade_stream);

    EXPECT_EQ(call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// subscribe_ticker
// ---------------------------------------------------------------------------
TEST(MarketStreamTest, SubscribeTickerConnectsToCorrectPath) {
    TestableMarketStream ms;
    ms.subscribe_ticker("BTCUSDT", [](const models::Ticker& /*t*/) {});
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@ticker");
}

TEST(MarketStreamTest, SubscribeTickerDispatchesTickerCallback) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};
    models::Ticker received;

    ms.subscribe_ticker("btcusdt", [&](const models::Ticker& t) {
        received = t;
        ++call_count;
    });

    ms.inject(fixtures::ws_ticker_stream);

    ASSERT_EQ(call_count.load(), 1);
    EXPECT_EQ(received.symbol, "BTCUSDT");
    EXPECT_NEAR(received.price, 42000.50, 0.001);
}

TEST(MarketStreamTest, SubscribeTickerIgnoresNonTickerEvents) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};

    ms.subscribe_ticker("btcusdt", [&](const models::Ticker& /*t*/) { ++call_count; });
    ms.inject(fixtures::ws_depth_stream);

    EXPECT_EQ(call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// subscribe_depth
// ---------------------------------------------------------------------------
TEST(MarketStreamTest, SubscribeDepthConnectsToCorrectPath) {
    TestableMarketStream ms;
    ms.subscribe_depth("BTCUSDT", [](const models::OrderBook& /*ob*/) {});
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@depth");
}

TEST(MarketStreamTest, SubscribeDepthDispatchesDepthCallback) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};
    models::OrderBook received;

    ms.subscribe_depth("btcusdt", [&](const models::OrderBook& ob) {
        received = ob;
        ++call_count;
    });

    ms.inject(fixtures::ws_depth_stream);

    ASSERT_EQ(call_count.load(), 1);
    ASSERT_EQ(received.bids.size(), 2UL);
    ASSERT_EQ(received.asks.size(), 2UL);
    EXPECT_NEAR(received.bids[0].price, 42000.00, 0.001);
}

TEST(MarketStreamTest, SubscribeDepthIgnoresNonDepthEvents) {
    TestableMarketStream ms;
    std::atomic<int> call_count{0};

    ms.subscribe_depth("btcusdt", [&](const models::OrderBook& /*ob*/) { ++call_count; });
    ms.inject(fixtures::ws_ticker_stream);

    EXPECT_EQ(call_count.load(), 0);
}

}  // namespace

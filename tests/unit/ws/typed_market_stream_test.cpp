// Unit tests for bintrade::ws::TypedMarketStream<Handler>
//
// Mirrors market_stream_test.cpp but uses CRTP handler dispatch instead of
// std::function callbacks. Verifies:
//   - Correct stream path per subscribe method
//   - Event routing to handler methods
//   - Silent discard of non-matching events and malformed JSON
//   - Partial handlers (only some on_* methods defined)

#include "binance_responses.hpp"

#include <bintrade/core/config.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/ws/typed_market_stream.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <string_view>

namespace {

using namespace bintrade;
using namespace bintrade::ws;
namespace fixtures = bintrade::test::fixtures;

// ---------------------------------------------------------------------------
// Full handler with all four on_* methods.
// ---------------------------------------------------------------------------
struct FullMarketHandler {
    std::atomic<int> trade_count{0};
    std::atomic<int> kline_count{0};
    std::atomic<int> ticker_count{0};
    std::atomic<int> order_book_count{0};

    models::Trade last_trade;
    models::Kline last_kline;
    models::Ticker last_ticker;
    models::OrderBook last_order_book;

    void on_trade(const models::Trade& t) {
        last_trade = t;
        ++trade_count;
    }
    void on_kline(const models::Kline& k) {
        last_kline = k;
        ++kline_count;
    }
    void on_ticker(const models::Ticker& t) {
        last_ticker = t;
        ++ticker_count;
    }
    void on_order_book(const models::OrderBook& ob) {
        last_order_book = ob;
        ++order_book_count;
    }
};

// ---------------------------------------------------------------------------
// Partial handler: only on_trade.
// ---------------------------------------------------------------------------
struct TradeOnlyHandler {
    std::atomic<int> trade_count{0};
    models::Trade last_trade;

    void on_trade(const models::Trade& t) {
        last_trade = t;
        ++trade_count;
    }
};

// ---------------------------------------------------------------------------
// TestableTypedMarketStream -- suppresses network I/O, records path.
// ---------------------------------------------------------------------------
template <typename Handler>
class TestableTypedMarketStream : public TypedMarketStream<Handler> {
public:
    using TypedMarketStream<Handler>::TypedMarketStream;

    std::string last_connected_path;

    void inject(std::string_view msg) { this->deliver_message(msg); }

protected:
    void do_connect(std::string_view stream_name) override { last_connected_path = std::string(stream_name); }
};

// ---------------------------------------------------------------------------
// subscribe_trades
// ---------------------------------------------------------------------------
TEST(TypedMarketStreamTest, SubscribeTradesConnectsToCorrectPath) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_trades("btcusdt");
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@trade");
}

TEST(TypedMarketStreamTest, SubscribeTradesNormalisesSymbolToLower) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_trades("BTCUSDT");
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@trade");
}

TEST(TypedMarketStreamTest, SubscribeTradesDispatchesTradeCallback) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_trades("btcusdt");

    ms.inject(fixtures::ws_trade_stream);

    ASSERT_EQ(h.trade_count.load(), 1);
    EXPECT_EQ(h.last_trade.id, 100'000'001UL);
    EXPECT_NEAR(h.last_trade.price, 42000.50, 0.001);
}

TEST(TypedMarketStreamTest, SubscribeTradesIgnoresNonTradeEvents) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_trades("btcusdt");
    ms.inject(fixtures::ws_kline_stream);
    EXPECT_EQ(h.trade_count.load(), 0);
}

TEST(TypedMarketStreamTest, SubscribeTradesIgnoresMalformedJson) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_trades("btcusdt");
    ms.inject("{{{not valid json");
    EXPECT_EQ(h.trade_count.load(), 0);
}

// ---------------------------------------------------------------------------
// subscribe_klines
// ---------------------------------------------------------------------------
TEST(TypedMarketStreamTest, SubscribeKlinesConnectsToCorrectPath) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_klines("BTCUSDT", "1h");
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@kline_1h");
}

TEST(TypedMarketStreamTest, SubscribeKlinesDispatchesKlineCallback) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_klines("btcusdt", "1h");

    ms.inject(fixtures::ws_kline_stream);

    ASSERT_EQ(h.kline_count.load(), 1);
    EXPECT_NEAR(h.last_kline.open, 42000.00, 0.001);
    EXPECT_EQ(h.last_kline.num_trades, 500UL);
}

TEST(TypedMarketStreamTest, SubscribeKlinesIgnoresNonKlineEvents) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_klines("btcusdt", "1h");
    ms.inject(fixtures::ws_trade_stream);
    EXPECT_EQ(h.kline_count.load(), 0);
}

// ---------------------------------------------------------------------------
// subscribe_ticker
// ---------------------------------------------------------------------------
TEST(TypedMarketStreamTest, SubscribeTickerConnectsToCorrectPath) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_ticker("BTCUSDT");
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@ticker");
}

TEST(TypedMarketStreamTest, SubscribeTickerDispatchesTickerCallback) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_ticker("btcusdt");

    ms.inject(fixtures::ws_ticker_stream);

    ASSERT_EQ(h.ticker_count.load(), 1);
    EXPECT_EQ(h.last_ticker.symbol, "BTCUSDT");
    EXPECT_NEAR(h.last_ticker.price, 42000.50, 0.001);
}

TEST(TypedMarketStreamTest, SubscribeTickerIgnoresNonTickerEvents) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_ticker("btcusdt");
    ms.inject(fixtures::ws_depth_stream);
    EXPECT_EQ(h.ticker_count.load(), 0);
}

// ---------------------------------------------------------------------------
// subscribe_depth
// ---------------------------------------------------------------------------
TEST(TypedMarketStreamTest, SubscribeDepthConnectsToCorrectPath) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_depth("BTCUSDT");
    EXPECT_EQ(ms.last_connected_path, "/ws/btcusdt@depth");
}

TEST(TypedMarketStreamTest, SubscribeDepthDispatchesDepthCallback) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_depth("btcusdt");

    ms.inject(fixtures::ws_depth_stream);

    ASSERT_EQ(h.order_book_count.load(), 1);
    ASSERT_EQ(h.last_order_book.bids.size(), 2UL);
    ASSERT_EQ(h.last_order_book.asks.size(), 2UL);
    EXPECT_NEAR(h.last_order_book.bids[0].price, 42000.00, 0.001);
}

TEST(TypedMarketStreamTest, SubscribeDepthIgnoresNonDepthEvents) {
    FullMarketHandler h;
    TestableTypedMarketStream<FullMarketHandler> ms(h);
    ms.subscribe_depth("btcusdt");
    ms.inject(fixtures::ws_ticker_stream);
    EXPECT_EQ(h.order_book_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Partial handler: only on_trade defined, other events silently skipped.
// ---------------------------------------------------------------------------
TEST(TypedMarketStreamTest, PartialHandlerCompilesAndWorks) {
    TradeOnlyHandler h;
    TestableTypedMarketStream<TradeOnlyHandler> ms(h);
    ms.subscribe_trades("btcusdt");

    ms.inject(fixtures::ws_trade_stream);
    ASSERT_EQ(h.trade_count.load(), 1);
    EXPECT_EQ(h.last_trade.id, 100'000'001UL);
}

TEST(TypedMarketStreamTest, PartialHandlerKlineIgnored) {
    // TradeOnlyHandler has no on_kline -- subscribe_klines should compile
    // and silently discard kline events.
    TradeOnlyHandler h;
    TestableTypedMarketStream<TradeOnlyHandler> ms(h);
    ms.subscribe_klines("btcusdt", "1h");
    ms.inject(fixtures::ws_kline_stream);
    // No crash, no callback.
    EXPECT_EQ(h.trade_count.load(), 0);
}

}  // namespace

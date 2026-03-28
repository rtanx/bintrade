// Unit tests for MarketStream DispatchMode (Inline vs Queued).
//
// Verifies:
//   - DispatchMode::Inline still works as before (default)
//   - DispatchMode::Queued delivers events via SPSC consumer thread
//   - Malformed JSON is silently discarded in queued mode
//   - set_dispatch_mode after subscribe throws ValidationException
//   - Consumer thread stops cleanly on destruction

#include "binance_responses.hpp"

#include <bintrade/core/config.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/ws/market_stream.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>

namespace {

using namespace bintrade;
using namespace bintrade::ws;
namespace fixtures = bintrade::test::fixtures;

// ---------------------------------------------------------------------------
// TestableMarketStream with dispatch mode support.
// ---------------------------------------------------------------------------
class TestableMarketStream : public MarketStream {
public:
    using MarketStream::MarketStream;

    std::string last_connected_path;

    void inject(std::string_view msg) { deliver_message(msg); }

protected:
    void do_connect(std::string_view stream_name) override { last_connected_path = std::string(stream_name); }
};

// Helper: wait for an atomic counter to reach expected value, with timeout.
bool wait_for(const std::atomic<int>& counter, int expected, std::chrono::milliseconds timeout = std::chrono::milliseconds{500}) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (counter.load() < expected) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return true;
}

// ---------------------------------------------------------------------------
// Inline mode (default) -- same as existing behaviour.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, InlineModeDefault) {
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
}

// ---------------------------------------------------------------------------
// Queued mode -- trades.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, QueuedModeDeliversTrade) {
    TestableMarketStream ms;
    ms.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> call_count{0};
    models::Trade received;

    ms.subscribe_trades("btcusdt", [&](const models::Trade& t) {
        received = t;
        ++call_count;
    });

    ms.inject(fixtures::ws_trade_stream);
    ASSERT_TRUE(wait_for(call_count, 1));
    EXPECT_EQ(received.id, 100'000'001UL);
    EXPECT_NEAR(received.price, 42000.50, 0.001);
}

// ---------------------------------------------------------------------------
// Queued mode -- klines.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, QueuedModeDeliversKline) {
    TestableMarketStream ms;
    ms.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> call_count{0};
    models::Kline received;

    ms.subscribe_klines("btcusdt", "1h", [&](const models::Kline& k) {
        received = k;
        ++call_count;
    });

    ms.inject(fixtures::ws_kline_stream);
    ASSERT_TRUE(wait_for(call_count, 1));
    EXPECT_NEAR(received.open, 42000.00, 0.001);
}

// ---------------------------------------------------------------------------
// Queued mode -- ticker.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, QueuedModeDeliversTicker) {
    TestableMarketStream ms;
    ms.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> call_count{0};
    models::Ticker received;

    ms.subscribe_ticker("btcusdt", [&](const models::Ticker& t) {
        received = t;
        ++call_count;
    });

    ms.inject(fixtures::ws_ticker_stream);
    ASSERT_TRUE(wait_for(call_count, 1));
    EXPECT_EQ(received.symbol, "BTCUSDT");
}

// ---------------------------------------------------------------------------
// Queued mode -- depth.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, QueuedModeDeliversDepth) {
    TestableMarketStream ms;
    ms.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> call_count{0};
    models::OrderBook received;

    ms.subscribe_depth("btcusdt", [&](const models::OrderBook& ob) {
        received = ob;
        ++call_count;
    });

    ms.inject(fixtures::ws_depth_stream);
    ASSERT_TRUE(wait_for(call_count, 1));
    ASSERT_EQ(received.bids.size(), 2UL);
}

// ---------------------------------------------------------------------------
// Queued mode ignores malformed JSON.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, QueuedModeIgnoresMalformedJson) {
    TestableMarketStream ms;
    ms.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> call_count{0};

    ms.subscribe_trades("btcusdt", [&](const models::Trade& /*t*/) { ++call_count; });
    ms.inject("{{{not valid json");

    // Give the consumer thread time to process.
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    EXPECT_EQ(call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Queued mode ignores non-matching event types.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, QueuedModeIgnoresNonMatchingEvent) {
    TestableMarketStream ms;
    ms.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> call_count{0};

    ms.subscribe_trades("btcusdt", [&](const models::Trade& /*t*/) { ++call_count; });
    ms.inject(fixtures::ws_kline_stream);

    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    EXPECT_EQ(call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// set_dispatch_mode after subscribe throws.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, SetDispatchModeAfterSubscribeThrows) {
    TestableMarketStream ms;
    ms.subscribe_trades("btcusdt", [](const models::Trade& /*t*/) {});
    EXPECT_THROW(ms.set_dispatch_mode(DispatchMode::Queued), ValidationException);
}

// ---------------------------------------------------------------------------
// Queued mode delivers multiple messages.
// ---------------------------------------------------------------------------
TEST(MarketStreamDispatchTest, QueuedModeMultipleMessages) {
    TestableMarketStream ms;
    ms.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> call_count{0};

    ms.subscribe_trades("btcusdt", [&](const models::Trade& /*t*/) { ++call_count; });

    for (int i = 0; i < 10; ++i) {
        ms.inject(fixtures::ws_trade_stream);
    }

    ASSERT_TRUE(wait_for(call_count, 10));
}

}  // namespace

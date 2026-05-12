// WebSocket integration test that connects to the Binance testnet stream
// endpoint, subscribes to BTCUSDT trades, waits for the first message, and
// verifies that the parsed trade has plausible price/qty values.
//
// Built only when -DBINTRADE_ENABLE_INTEGRATION_TESTS=ON.

#include <bintrade/models/trade.hpp>
#include <bintrade/ws/market_stream.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <mutex>

namespace bintrade::test {

namespace {

constexpr auto k_deadline = std::chrono::seconds(10);
// Binance testnet has separate REST and WS subdomains.  The streams live
// behind stream.testnet.binance.vision on port 9443, mirroring the
// production stream.binance.com:9443 layout.
constexpr const char* k_testnet_ws_url = "wss://stream.testnet.binance.vision:9443";

}  // namespace

TEST(WsIntegrationTest, ReceivesAtLeastOneTradeFromTestnet) {
    WebSocketConfig config;
    config.use_testnet = true;
    config.base_url = k_testnet_ws_url;

    ws::MarketStream stream(config);

    std::promise<models::Trade> first_trade_promise;
    auto first_trade_future = first_trade_promise.get_future();

    // Guard against the (extremely unlikely) case where the testnet pushes
    // more than one trade before set_value() returns -- promise can only be
    // satisfied once.
    std::once_flag fulfilled;

    stream.subscribe_trades("BTCUSDT", [&](const models::Trade& trade) { std::call_once(fulfilled, [&] { first_trade_promise.set_value(trade); }); });

    auto status = first_trade_future.wait_for(k_deadline);
    ASSERT_EQ(status, std::future_status::ready) << "did not receive any trade within " << k_deadline.count() << " seconds";

    auto trade = first_trade_future.get();
    EXPECT_GT(trade.price, 0.0);
    EXPECT_GT(trade.qty, 0.0);
}

}  // namespace bintrade::test

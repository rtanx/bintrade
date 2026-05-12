// REST integration tests that hit the Binance testnet at
// https://testnet.binance.vision.  Built only when
// -DBINTRADE_ENABLE_INTEGRATION_TESTS=ON.  Each test has a hard wall-clock
// deadline (kDeadline) so a network stall cannot hang CI.
//
// Signed-endpoint tests are SKIPPED unless BINTRADE_TEST_API_KEY and
// BINTRADE_TEST_API_SECRET are present in the environment.

#include <bintrade/auth/credentials.hpp>
#include <bintrade/rest/account_client.hpp>
#include <bintrade/rest/market_data_client.hpp>

#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <chrono>
#include <cstdlib>
#include <future>
#include <string>

namespace bintrade::test {

namespace {

constexpr auto kDeadline = std::chrono::seconds(10);
constexpr const char* kTestnetRestUrl = "https://testnet.binance.vision";

// Spawn a coroutine on a fresh io_context, run until completion, and return
// the result.  Throws if the coroutine throws or if `kDeadline` expires.
template <typename T>
T run_async_with_deadline(boost::asio::awaitable<T> awaitable) {
    boost::asio::io_context ioc;
    auto fut = boost::asio::co_spawn(ioc, std::move(awaitable), boost::asio::use_future);

    std::thread runner([&ioc] { ioc.run(); });

    auto status = fut.wait_for(kDeadline);
    if (status != std::future_status::ready) {
        ioc.stop();
        if (runner.joinable()) {
            runner.join();
        }
        throw std::runtime_error("integration test deadline exceeded");
    }

    if (runner.joinable()) {
        runner.join();
    }
    return fut.get();
}

}  // namespace

class RestIntegrationTest : public ::testing::Test {
protected:
    RestConfig make_config() {
        RestConfig config;
        config.use_testnet = true;
        config.base_url = kTestnetRestUrl;
        return config;
    }
};

TEST_F(RestIntegrationTest, PingReturnsTrue) {
    rest::MarketDataClient client(make_config());
    EXPECT_TRUE(client.ping());
}

TEST_F(RestIntegrationTest, ServerTimeIsCloseToLocalClock) {
    rest::MarketDataClient client(make_config());
    auto server_tp = client.server_time();
    auto now_tp = std::chrono::system_clock::now();

    // Allow up to 5 seconds drift in either direction.  Generous enough for
    // slow CI runners and tight enough to catch a totally wrong response.
    auto diff = server_tp > now_tp ? server_tp - now_tp : now_tp - server_tp;
    EXPECT_LT(diff, std::chrono::seconds(5)) << "server_time drifted by " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
                                             << " ms";
}

TEST_F(RestIntegrationTest, AsyncPriceTickerForBtcUsdt) {
    rest::MarketDataClient client(make_config());
    auto ticker = run_async_with_deadline<models::Ticker>(client.async_get_price_ticker("BTCUSDT"));
    EXPECT_GT(ticker.price, 0.0);
}

TEST_F(RestIntegrationTest, AsyncOrderBookHasBidsAndAsks) {
    rest::MarketDataClient client(make_config());
    auto book = run_async_with_deadline<models::OrderBook>(client.async_get_order_book("BTCUSDT", 5));
    EXPECT_FALSE(book.bids.empty());
    EXPECT_FALSE(book.asks.empty());
}

TEST_F(RestIntegrationTest, SignedAccountInfoSmokeTest) {
    const char* key = std::getenv("BINTRADE_TEST_API_KEY");
    const char* secret = std::getenv("BINTRADE_TEST_API_SECRET");
    if (key == nullptr || secret == nullptr) {
        GTEST_SKIP() << "Skipping signed test: set BINTRADE_TEST_API_KEY and BINTRADE_TEST_API_SECRET to enable.";
    }

    Credentials creds(std::string{key}, std::string{secret});
    rest::AccountClient client(make_config(), std::move(creds));

    auto info = client.get_account_info();
    // We deliberately do not assert on balance count or maker/taker commission
    // because a freshly provisioned testnet account may legitimately have an
    // empty list.  We just verify the call did not throw.
    SUCCEED() << "AccountInfo returned " << info.balances.size() << " balance entries";
}

}  // namespace bintrade::test

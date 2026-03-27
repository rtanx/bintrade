// Tests for the async API of Client, MarketDataClient, TradingClient, and
// AccountClient.
//
// Strategy: inject MockHttpTransport (unchanged -- async_get/post/del default
// implementations call the synchronous GMock overrides directly).  Coroutines
// are driven by a per-test asio::io_context: co_spawn(..., use_future) yields
// a std::future<T>; ioc.run() processes the handler; future.get() returns the
// result or re-throws any stored exception.

#include "binance_responses.hpp"
#include "mock_http_transport.hpp"
#include "rest/detail/http_transport.hpp"

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/core/types.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/rest/account_client.hpp>
#include <bintrade/rest/market_data_client.hpp>
#include <bintrade/rest/trading_client.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <chrono>
#include <future>
#include <memory>
#include <string>

using namespace testing;
using namespace bintrade;
using namespace bintrade::rest;
using bintrade::rest::detail::HttpResponse;
using bintrade::rest::detail::HttpTransport;
using bintrade::test::MockHttpTransport;
namespace asio = boost::asio;

namespace {

// ---------------------------------------------------------------------------
// Testable wrappers expose the protected transport-injection constructors.
// ---------------------------------------------------------------------------

class TestableClient : public Client {
public:
    explicit TestableClient(std::unique_ptr<HttpTransport> t) : Client(std::move(t)) {}
    TestableClient(std::unique_ptr<HttpTransport> t, Credentials c) : Client(std::move(t), std::move(c)) {}
};

class TestableMarketDataClient : public MarketDataClient {
public:
    explicit TestableMarketDataClient(std::unique_ptr<HttpTransport> t) : MarketDataClient(std::move(t)) {}
};

class TestableTradingClient : public TradingClient {
public:
    explicit TestableTradingClient(std::unique_ptr<HttpTransport> t) : TradingClient(std::move(t)) {}
    TestableTradingClient(std::unique_ptr<HttpTransport> t, Credentials c) : TradingClient(std::move(t), std::move(c)) {}
};

class TestableAccountClient : public AccountClient {
public:
    explicit TestableAccountClient(std::unique_ptr<HttpTransport> t) : AccountClient(std::move(t)) {}
    TestableAccountClient(std::unique_ptr<HttpTransport> t, Credentials c) : AccountClient(std::move(t), std::move(c)) {}
};

// ---------------------------------------------------------------------------
// Helper: run a single awaitable on a fresh io_context and return its result.
// ---------------------------------------------------------------------------

template <typename T>
T run_async(asio::awaitable<T> aw) {
    asio::io_context ioc;
    auto fut = asio::co_spawn(ioc, std::move(aw), asio::use_future);
    ioc.run();
    return fut.get();
}

// ---------------------------------------------------------------------------
// Fixture for signed-endpoint tests (NiceMock suppresses set_api_key call).
// ---------------------------------------------------------------------------

template <typename ClientT>
struct AuthFixture {
    NiceMock<MockHttpTransport>* raw{};
    ClientT client;

    AuthFixture()
        : client([this]() -> ClientT {
              auto mock = std::make_unique<NiceMock<MockHttpTransport>>();
              raw = mock.get();
              return {std::move(mock), Credentials("test-api-key", "test-secret-32chars-padded000")};
          }()) {}
};

// ===========================================================================
// Client::async_ping
// ===========================================================================

TEST(AsyncClientPingTest, ReturnsTrueWhenStatusIs200) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ping", IsEmpty())).WillOnce(Return(HttpResponse{200, "{}"}));
    TestableClient client(std::move(mock));

    EXPECT_TRUE(run_async(client.async_ping()));
}

TEST(AsyncClientPingTest, ReturnsFalseWhenStatusIsNot200) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ping", _)).WillOnce(Return(HttpResponse{503, ""}));
    TestableClient client(std::move(mock));

    EXPECT_FALSE(run_async(client.async_ping()));
}

// ===========================================================================
// Client::async_server_time
// ===========================================================================

TEST(AsyncClientServerTimeTest, ParsesServerTime) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/time", IsEmpty())).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::server_time_response)}));
    TestableClient client(std::move(mock));

    auto ts = run_async(client.async_server_time());
    using namespace std::chrono;
    auto ms = duration_cast<milliseconds>(ts.time_since_epoch());
    EXPECT_EQ(ms.count(), 1'704'067'200'000LL);
}

TEST(AsyncClientServerTimeTest, ThrowsApiExceptionOnErrorResponse) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/time", _)).WillOnce(Return(HttpResponse{400, std::string(test::fixtures::api_error_response)}));
    TestableClient client(std::move(mock));

    EXPECT_THROW(run_async(client.async_server_time()), ApiException);
}

// ===========================================================================
// MarketDataClient async methods
// ===========================================================================

TEST(AsyncMarketDataClientTest, AsyncGetPriceTickerCallsCorrectEndpoint) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/price", Contains(Pair("symbol", "BTCUSDT"))))
        .WillOnce(Return(HttpResponse{200, std::string(test::fixtures::ticker_response)}));
    TestableMarketDataClient client(std::move(mock));

    auto ticker = run_async(client.async_get_price_ticker("BTCUSDT"));
    EXPECT_EQ(ticker.symbol, "BTCUSDT");
    EXPECT_DOUBLE_EQ(ticker.price, 42000.50);
}

TEST(AsyncMarketDataClientTest, AsyncGetOrderBookParsesResult) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/depth", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::order_book_response)}));
    TestableMarketDataClient client(std::move(mock));

    auto book = run_async(client.async_get_order_book("BTCUSDT"));
    EXPECT_EQ(book.symbol, "BTCUSDT");
    EXPECT_EQ(book.last_update_id, 12'345'678ULL);
    ASSERT_EQ(book.bids.size(), 2U);
    EXPECT_DOUBLE_EQ(book.bids[0].price, 42000.0);
}

TEST(AsyncMarketDataClientTest, AsyncGetAllPriceTickersReturnsList) {
    // Return a JSON array with one ticker.
    const std::string body = R"([{"symbol":"BTCUSDT","price":"42000.50"}])";
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/price", IsEmpty())).WillOnce(Return(HttpResponse{200, body}));
    TestableMarketDataClient client(std::move(mock));

    auto tickers = run_async(client.async_get_all_price_tickers());
    ASSERT_EQ(tickers.size(), 1U);
    EXPECT_EQ(tickers[0].symbol, "BTCUSDT");
}

TEST(AsyncMarketDataClientTest, AsyncGetPriceTickerThrowsOnApiError) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/price", _)).WillOnce(Return(HttpResponse{400, std::string(test::fixtures::api_error_response)}));
    TestableMarketDataClient client(std::move(mock));

    EXPECT_THROW(run_async(client.async_get_price_ticker("INVALID")), ApiException);
}

// ===========================================================================
// TradingClient async methods
// ===========================================================================

TEST(AsyncTradingClientTest, AsyncNewOrderLimitBuyParsesOrder) {
    AuthFixture<TestableTradingClient> fx;
    EXPECT_CALL(*fx.raw, post("/api/v3/order", _, _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::new_order_response)}));

    models::NewOrderRequest req;
    req.symbol = "BTCUSDT";
    req.side = Side::Buy;
    req.type = OrderType::Limit;
    req.quantity = 1.0;
    req.price = 42000.0;

    auto order = run_async(fx.client.async_new_order(std::move(req)));
    EXPECT_EQ(order.symbol, "BTCUSDT");
    EXPECT_EQ(order.order_id, 123'456'789ULL);
    EXPECT_EQ(order.status, OrderStatus::New);
}

TEST(AsyncTradingClientTest, AsyncCancelOrderParsesOrder) {
    AuthFixture<TestableTradingClient> fx;
    EXPECT_CALL(*fx.raw, del("/api/v3/order", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::cancel_order_response)}));

    models::CancelOrderRequest req;
    req.symbol = "BTCUSDT";
    req.order_id = 123'456'789ULL;

    auto order = run_async(fx.client.async_cancel_order(std::move(req)));
    EXPECT_EQ(order.status, OrderStatus::Canceled);
}

TEST(AsyncTradingClientTest, AsyncGetOpenOrdersReturnsList) {
    AuthFixture<TestableTradingClient> fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/openOrders", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::open_orders_response)}));

    auto orders = run_async(fx.client.async_get_open_orders("BTCUSDT"));
    ASSERT_EQ(orders.size(), 1U);
    EXPECT_EQ(orders[0].symbol, "BTCUSDT");
    EXPECT_EQ(orders[0].status, OrderStatus::New);
}

TEST(AsyncTradingClientTest, AsyncNewOrderThrowsOnApiError) {
    AuthFixture<TestableTradingClient> fx;
    EXPECT_CALL(*fx.raw, post("/api/v3/order", _, _)).WillOnce(Return(HttpResponse{400, std::string(test::fixtures::api_error_response)}));

    models::NewOrderRequest req;
    req.symbol = "INVALID";
    req.side = Side::Buy;
    req.type = OrderType::Market;
    req.quantity = 1.0;

    EXPECT_THROW(run_async(fx.client.async_new_order(std::move(req))), ApiException);
}

// ===========================================================================
// AccountClient async methods
// ===========================================================================

TEST(AsyncAccountClientTest, AsyncGetAccountInfoParsesResult) {
    AuthFixture<TestableAccountClient> fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/account", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::account_info_response)}));

    auto info = run_async(fx.client.async_get_account_info());
    EXPECT_TRUE(info.can_trade);
    EXPECT_EQ(info.account_type, "SPOT");
    // Fixture has 2 non-zero balances (BTC + USDT); ETH is skipped.
    EXPECT_EQ(info.balances.size(), 2U);
}

TEST(AsyncAccountClientTest, AsyncGetAccountTradesParsesResult) {
    AuthFixture<TestableAccountClient> fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/myTrades", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::account_trades_response)}));

    auto trades = run_async(fx.client.async_get_account_trades("BTCUSDT"));
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].symbol, "BTCUSDT");
    EXPECT_TRUE(trades[0].is_buyer);
}

TEST(AsyncAccountClientTest, AsyncGetAccountInfoThrowsOnApiError) {
    AuthFixture<TestableAccountClient> fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/account", _)).WillOnce(Return(HttpResponse{400, std::string(test::fixtures::api_error_response)}));

    EXPECT_THROW(run_async(fx.client.async_get_account_info()), ApiException);
}

// ===========================================================================
// Concurrent async requests
// ===========================================================================

TEST(AsyncConcurrentTest, MultipleRequestsOnSameIoContext) {
    // Verify that spawning multiple coroutines on the same io_context works.
    auto mock = std::make_unique<NiceMock<MockHttpTransport>>();
    auto* raw = mock.get();
    TestableMarketDataClient client(std::move(mock));

    EXPECT_CALL(*raw, get("/api/v3/ticker/price", _))
        .Times(3)
        .WillRepeatedly(Return(HttpResponse{200, std::string(test::fixtures::ticker_response)}));

    asio::io_context ioc;
    auto fut1 = asio::co_spawn(ioc, client.async_get_price_ticker("BTCUSDT"), asio::use_future);
    auto fut2 = asio::co_spawn(ioc, client.async_get_price_ticker("BTCUSDT"), asio::use_future);
    auto fut3 = asio::co_spawn(ioc, client.async_get_price_ticker("BTCUSDT"), asio::use_future);

    ioc.run();

    EXPECT_EQ(fut1.get().symbol, "BTCUSDT");
    EXPECT_EQ(fut2.get().symbol, "BTCUSDT");
    EXPECT_EQ(fut3.get().symbol, "BTCUSDT");
}

}  // namespace

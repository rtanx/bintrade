// Tests for bintrade::rest::MarketDataClient -- all 6 public methods.
//
// Uses MockHttpTransport to verify correct endpoint paths, fixture JSON
// parsing, parameter forwarding, and error propagation.

#include "binance_responses.hpp"
#include "mock_http_transport.hpp"
#include "rest/detail/http_transport.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/rest/market_data_client.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace testing;
using namespace bintrade;
using namespace bintrade::rest;
using bintrade::rest::detail::HttpResponse;
using bintrade::rest::detail::HttpTransport;
using bintrade::test::MockHttpTransport;

namespace {

class TestableMarketDataClient : public MarketDataClient {
public:
    explicit TestableMarketDataClient(std::unique_ptr<HttpTransport> t) : MarketDataClient(std::move(t)) {}
};

// ---------------------------------------------------------------------------
// get_order_book
// ---------------------------------------------------------------------------
TEST(MarketDataClientTest, GetOrderBookCallsCorrectEndpoint) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/depth", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::order_book_response)}));
    TestableMarketDataClient client(std::move(mock));

    auto book = client.get_order_book("BTCUSDT");
    EXPECT_EQ(book.symbol, "BTCUSDT");
    EXPECT_EQ(book.last_update_id, 12'345'678ULL);
    ASSERT_EQ(book.bids.size(), 2U);
    ASSERT_EQ(book.asks.size(), 2U);
    EXPECT_DOUBLE_EQ(book.bids[0].price, 42000.0);
    EXPECT_DOUBLE_EQ(book.bids[0].quantity, 1.5);
    EXPECT_DOUBLE_EQ(book.asks[0].price, 42001.0);
}

TEST(MarketDataClientTest, GetOrderBookPassesSymbolAndLimit) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/depth", AllOf(Contains(Key("symbol")), Contains(Key("limit")))))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::order_book_response)}));
    TestableMarketDataClient client(std::move(mock));
    [[maybe_unused]] auto result = client.get_order_book("BTCUSDT", 10);
}

TEST(MarketDataClientTest, GetOrderBookThrowsOnApiError) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/depth", _))
        .WillOnce(Return(HttpResponse{.status_code = 400, .body = std::string(test::fixtures::api_error_response)}));
    TestableMarketDataClient client(std::move(mock));

    EXPECT_THROW((void)client.get_order_book("INVALIDXYZ"), ApiException);
}

// ---------------------------------------------------------------------------
// get_recent_trades
// ---------------------------------------------------------------------------
TEST(MarketDataClientTest, GetRecentTradesCallsCorrectEndpoint) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/trades", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::recent_trades_response)}));
    TestableMarketDataClient client(std::move(mock));

    auto trades = client.get_recent_trades("BTCUSDT");
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].id, 100'000'001ULL);
    EXPECT_DOUBLE_EQ(trades[0].price, 42000.5);
    EXPECT_DOUBLE_EQ(trades[0].qty, 0.123);
    EXPECT_FALSE(trades[0].is_buyer);
}

TEST(MarketDataClientTest, GetRecentTradesPassesSymbolAndLimit) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/trades", AllOf(Contains(Key("symbol")), Contains(Key("limit")))))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = "[]"}));
    TestableMarketDataClient client(std::move(mock));

    auto trades = client.get_recent_trades("BTCUSDT", 50);
    EXPECT_TRUE(trades.empty());
}

// ---------------------------------------------------------------------------
// get_klines
// ---------------------------------------------------------------------------
TEST(MarketDataClientTest, GetKlinesCallsCorrectEndpoint) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/klines", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::klines_response)}));
    TestableMarketDataClient client(std::move(mock));

    auto klines = client.get_klines("BTCUSDT", "1h");
    ASSERT_EQ(klines.size(), 1U);
    EXPECT_DOUBLE_EQ(klines[0].open, 42000.0);
    EXPECT_DOUBLE_EQ(klines[0].high, 43000.0);
    EXPECT_DOUBLE_EQ(klines[0].low, 41000.0);
    EXPECT_DOUBLE_EQ(klines[0].close, 42500.0);
    EXPECT_DOUBLE_EQ(klines[0].volume, 100.5);
    EXPECT_EQ(klines[0].num_trades, 500ULL);
}

TEST(MarketDataClientTest, GetKlinesPassesSymbolIntervalLimit) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/klines", AllOf(Contains(Key("symbol")), Contains(Key("interval")), Contains(Key("limit")))))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = "[]"}));
    TestableMarketDataClient client(std::move(mock));
    [[maybe_unused]] auto result = client.get_klines("ETHUSDT", "4h", 100);
}

// ---------------------------------------------------------------------------
// get_ticker_24h
// ---------------------------------------------------------------------------
TEST(MarketDataClientTest, GetTicker24hCallsCorrectEndpoint) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/24hr", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::ticker24h_response)}));
    TestableMarketDataClient client(std::move(mock));

    auto ticker = client.get_ticker_24h("BTCUSDT");
    EXPECT_EQ(ticker.symbol, "BTCUSDT");
    EXPECT_DOUBLE_EQ(ticker.price_change, 1500.0);
    EXPECT_DOUBLE_EQ(ticker.open, 40500.0);
    EXPECT_DOUBLE_EQ(ticker.high, 43000.0);
    EXPECT_DOUBLE_EQ(ticker.low, 40000.0);
    EXPECT_DOUBLE_EQ(ticker.close, 42000.5);
    EXPECT_EQ(ticker.num_trades, 987'654ULL);
}

// ---------------------------------------------------------------------------
// get_price_ticker
// ---------------------------------------------------------------------------
TEST(MarketDataClientTest, GetPriceTickerCallsCorrectEndpoint) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/price", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::ticker_response)}));
    TestableMarketDataClient client(std::move(mock));

    auto ticker = client.get_price_ticker("BTCUSDT");
    EXPECT_EQ(ticker.symbol, "BTCUSDT");
    EXPECT_DOUBLE_EQ(ticker.price, 42000.5);
}

TEST(MarketDataClientTest, GetPriceTickerThrowsApiExceptionOn400) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/price", _))
        .WillOnce(Return(HttpResponse{.status_code = 400, .body = std::string(test::fixtures::api_error_response)}));
    TestableMarketDataClient client(std::move(mock));

    EXPECT_THROW((void)client.get_price_ticker("BADXYZ"), ApiException);
}

// ---------------------------------------------------------------------------
// get_all_price_tickers
// ---------------------------------------------------------------------------
TEST(MarketDataClientTest, GetAllPriceTickersCallsCorrectEndpointWithNoParams) {
    const std::string all_tickers_json = R"([{"symbol":"BTCUSDT","price":"42000.50"},{"symbol":"ETHUSDT","price":"2800.00"}])";
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/price", IsEmpty())).WillOnce(Return(HttpResponse{.status_code = 200, .body = all_tickers_json}));
    TestableMarketDataClient client(std::move(mock));

    auto tickers = client.get_all_price_tickers();
    ASSERT_EQ(tickers.size(), 2U);
    EXPECT_EQ(tickers[0].symbol, "BTCUSDT");
    EXPECT_DOUBLE_EQ(tickers[0].price, 42000.5);
    EXPECT_EQ(tickers[1].symbol, "ETHUSDT");
    EXPECT_DOUBLE_EQ(tickers[1].price, 2800.0);
}

TEST(MarketDataClientTest, GetAllPriceTickersReturnsEmptyVectorForEmptyArray) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ticker/price", _)).WillOnce(Return(HttpResponse{.status_code = 200, .body = "[]"}));
    TestableMarketDataClient client(std::move(mock));

    EXPECT_TRUE(client.get_all_price_tickers().empty());
}

}  // namespace

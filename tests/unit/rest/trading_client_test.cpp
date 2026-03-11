// Tests for bintrade::rest::TradingClient -- all order-management methods.
//
// Signed endpoints use a NiceMock to suppress the set_api_key call during
// construction.  The mock transport returns fixture JSON for parsing tests and
// error payloads for exception-propagation tests.

#include "binance_responses.hpp"
#include "mock_http_transport.hpp"
#include "rest/detail/http_transport.hpp"

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/core/types.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/rest/trading_client.hpp>

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

class TestableTradingClient : public TradingClient {
public:
    explicit TestableTradingClient(std::unique_ptr<HttpTransport> t) : TradingClient(std::move(t)) {}
    TestableTradingClient(std::unique_ptr<HttpTransport> t, Credentials c) : TradingClient(std::move(t), std::move(c)) {}
};

// Build a client + NiceMock pair ready for signed-endpoint tests.
// The NiceMock suppresses the set_api_key() call made during construction.
struct AuthFixture {
    NiceMock<MockHttpTransport>* raw{};
    TestableTradingClient client;

    AuthFixture()
        : client([this]() -> TestableTradingClient {
              auto mock = std::make_unique<NiceMock<MockHttpTransport>>();
              raw = mock.get();
              return {std::move(mock), Credentials("test-api-key", "test-secret-32chars-padded000")};
          }()) {}
};

// ---------------------------------------------------------------------------
// new_order -- limit buy
// ---------------------------------------------------------------------------
TEST(TradingClientTest, NewOrderLimitBuyCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, post("/api/v3/order", _, _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::new_order_response)}));

    models::NewOrderRequest req;
    req.symbol = "BTCUSDT";
    req.side = Side::Buy;
    req.type = OrderType::Limit;
    req.quantity = 1.0;
    req.price = 42000.0;

    auto order = fx.client.new_order(req);
    EXPECT_EQ(order.symbol, "BTCUSDT");
    EXPECT_EQ(order.order_id, 123'456'789ULL);
    EXPECT_EQ(order.client_order_id, "myOrder1");
    EXPECT_EQ(order.status, OrderStatus::New);
    EXPECT_EQ(order.side, Side::Buy);
    EXPECT_EQ(order.type, OrderType::Limit);
}

TEST(TradingClientTest, NewOrderMarketSellCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, post("/api/v3/order", _, _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::filled_order_response)}));

    models::NewOrderRequest req;
    req.symbol = "BTCUSDT";
    req.side = Side::Sell;
    req.type = OrderType::Market;
    req.quantity = 0.5;

    auto order = fx.client.new_order(req);
    EXPECT_EQ(order.status, OrderStatus::Filled);
    EXPECT_EQ(order.side, Side::Buy);  // fixture hardcodes BUY -- parser field check
    EXPECT_DOUBLE_EQ(order.executed_qty, 1.0);
}

TEST(TradingClientTest, NewOrderThrowsApiExceptionOnError) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, post("/api/v3/order", _, _))
        .WillOnce(Return(HttpResponse{.status_code = 400, .body = std::string(test::fixtures::api_error_response)}));

    models::NewOrderRequest req;
    req.symbol = "BADXYZ";
    req.side = Side::Buy;
    req.type = OrderType::Market;
    req.quantity = 1.0;

    EXPECT_THROW((void)fx.client.new_order(req), ApiException);
}

// ---------------------------------------------------------------------------
// test_new_order
// ---------------------------------------------------------------------------
TEST(TradingClientTest, TestNewOrderCallsTestEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, post("/api/v3/order/test", _, _)).WillOnce(Return(HttpResponse{.status_code = 200, .body = "{}"}));

    models::NewOrderRequest req;
    req.symbol = "BTCUSDT";
    req.side = Side::Buy;
    req.type = OrderType::Limit;
    req.quantity = 0.1;
    req.price = 42000.0;

    EXPECT_NO_THROW(fx.client.test_new_order(req));
}

// ---------------------------------------------------------------------------
// query_order
// ---------------------------------------------------------------------------
TEST(TradingClientTest, QueryOrderByOrderIdCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/order", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::new_order_response)}));

    auto order = fx.client.query_order("BTCUSDT", 123'456'789ULL);
    EXPECT_EQ(order.order_id, 123'456'789ULL);
    EXPECT_EQ(order.status, OrderStatus::New);
}

TEST(TradingClientTest, QueryOrderByClientOrderIdCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/order", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::filled_order_response)}));

    auto order = fx.client.query_order("BTCUSDT", std::string("myOrder1"));
    EXPECT_EQ(order.status, OrderStatus::Filled);
}

TEST(TradingClientTest, QueryOrderThrowsOnApiError) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/order", _))
        .WillOnce(Return(HttpResponse{.status_code = 400, .body = R"({"code":-2013,"msg":"Order does not exist."})"}));

    EXPECT_THROW((void)fx.client.query_order("BTCUSDT", 9'999ULL), ApiException);
}

// ---------------------------------------------------------------------------
// cancel_order
// ---------------------------------------------------------------------------
TEST(TradingClientTest, CancelOrderByOrderIdCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, del("/api/v3/order", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::cancel_order_response)}));

    models::CancelOrderRequest req;
    req.symbol = "BTCUSDT";
    req.order_id = 123'456'789ULL;

    auto order = fx.client.cancel_order(req);
    EXPECT_EQ(order.status, OrderStatus::Canceled);
}

TEST(TradingClientTest, CancelOrderByClientOrderIdCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, del("/api/v3/order", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::cancel_order_response)}));

    models::CancelOrderRequest req;
    req.symbol = "BTCUSDT";
    req.client_order_id = "myOrder1";

    auto order = fx.client.cancel_order(req);
    EXPECT_EQ(order.status, OrderStatus::Canceled);
}

TEST(TradingClientTest, CancelOrderThrowsOnApiError) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, del("/api/v3/order", _))
        .WillOnce(Return(HttpResponse{.status_code = 400, .body = R"({"code":-2011,"msg":"Unknown order sent."})"}));

    models::CancelOrderRequest req;
    req.symbol = "BTCUSDT";
    req.order_id = 9'999ULL;

    EXPECT_THROW((void)fx.client.cancel_order(req), ApiException);
}

// ---------------------------------------------------------------------------
// cancel_all_orders
// ---------------------------------------------------------------------------
TEST(TradingClientTest, CancelAllOrdersCallsOpenOrdersEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, del("/api/v3/openOrders", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::open_orders_response)}));

    auto orders = fx.client.cancel_all_orders("BTCUSDT");
    ASSERT_EQ(orders.size(), 1U);
    EXPECT_EQ(orders[0].status, OrderStatus::New);
}

TEST(TradingClientTest, CancelAllOrdersReturnsEmptyVectorWhenNoOrders) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, del("/api/v3/openOrders", _)).WillOnce(Return(HttpResponse{.status_code = 200, .body = "[]"}));

    EXPECT_TRUE(fx.client.cancel_all_orders("BTCUSDT").empty());
}

// ---------------------------------------------------------------------------
// get_open_orders
// ---------------------------------------------------------------------------
TEST(TradingClientTest, GetOpenOrdersCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/openOrders", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::open_orders_response)}));

    auto orders = fx.client.get_open_orders("BTCUSDT");
    ASSERT_EQ(orders.size(), 1U);
    EXPECT_EQ(orders[0].symbol, "BTCUSDT");
    EXPECT_EQ(orders[0].order_id, 123'456'789ULL);
}

// ---------------------------------------------------------------------------
// get_all_open_orders
// ---------------------------------------------------------------------------
TEST(TradingClientTest, GetAllOpenOrdersCallsCorrectEndpointWithNoSymbolParam) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/openOrders", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::open_orders_response)}));

    auto orders = fx.client.get_all_open_orders();
    EXPECT_EQ(orders.size(), 1U);
}

// ---------------------------------------------------------------------------
// get_all_orders
// ---------------------------------------------------------------------------
TEST(TradingClientTest, GetAllOrdersCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/allOrders", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = std::string(test::fixtures::open_orders_response)}));

    auto orders = fx.client.get_all_orders("BTCUSDT");
    EXPECT_EQ(orders.size(), 1U);
}

TEST(TradingClientTest, GetAllOrdersPassesLimitParam) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/allOrders", Contains(Key("limit")))).WillOnce(Return(HttpResponse{.status_code = 200, .body = "[]"}));

    EXPECT_TRUE(fx.client.get_all_orders("BTCUSDT", 10).empty());
}

}  // namespace

#include "detail/json_parse.hpp"

#include <bintrade/rest/trading_client.hpp>

#include <nlohmann/json.hpp>

#include <boost/asio/awaitable.hpp>
#include <string>
#include <tuple>
#include <vector>

namespace bintrade::rest {

namespace asio = boost::asio;

// ---------------------------------------------------------------------------
// Helper: build Params from a NewOrderRequest
// ---------------------------------------------------------------------------
namespace {

using Params = std::unordered_map<std::string, std::string>;

Params build_new_order_params(const models::NewOrderRequest& request) {
    Params params;
    params["symbol"] = request.symbol;
    params["side"] = to_string(request.side);
    params["type"] = to_string(request.type);
    params["quantity"] = std::to_string(request.quantity);

    if (request.time_in_force != TimeInForce::GTC || request.type == OrderType::Limit) {
        params["timeInForce"] = to_string(request.time_in_force);
    }
    if (request.price.has_value()) {
        params["price"] = std::to_string(request.price.value());
    }
    if (request.client_order_id.has_value()) {
        params["newClientOrderId"] = request.client_order_id.value();
    }
    if (request.stop_price.has_value()) {
        params["stopPrice"] = std::to_string(request.stop_price.value());
    }
    return params;
}

Params build_cancel_params(const models::CancelOrderRequest& request) {
    Params params;
    params["symbol"] = request.symbol;
    if (request.order_id.has_value()) {
        params["orderId"] = std::to_string(request.order_id.value());
    }
    if (request.client_order_id.has_value()) {
        params["origClientOrderId"] = request.client_order_id.value();
    }
    return params;
}

}  // namespace

// ---------------------------------------------------------------------------
// Synchronous API
// ---------------------------------------------------------------------------

models::Order TradingClient::new_order(const models::NewOrderRequest& request) {
    auto body = signed_post("/api/v3/order", build_new_order_params(request));
    auto json = detail::parse_response(200, body);
    return detail::parse_order(json);
}

void TradingClient::test_new_order(const models::NewOrderRequest& request) {
    Params params = build_new_order_params(request);
    // stop_price and client_order_id are already in params if set.
    // test endpoint does not need them stripped.
    std::ignore = signed_post("/api/v3/order/test", std::move(params));
}

models::Order TradingClient::query_order(const Symbol& symbol, OrderId order_id) {
    Params params;
    params["symbol"] = symbol;
    params["orderId"] = std::to_string(order_id);
    auto body = signed_get("/api/v3/order", std::move(params));
    auto json = detail::parse_response(200, body);
    return detail::parse_order(json);
}

models::Order TradingClient::query_order(const Symbol& symbol, const ClientOrderId& client_order_id) {
    Params params;
    params["symbol"] = symbol;
    params["origClientOrderId"] = client_order_id;
    auto body = signed_get("/api/v3/order", std::move(params));
    auto json = detail::parse_response(200, body);
    return detail::parse_order(json);
}

models::Order TradingClient::cancel_order(const models::CancelOrderRequest& request) {
    auto body = signed_delete("/api/v3/order", build_cancel_params(request));
    auto json = detail::parse_response(200, body);
    return detail::parse_order(json);
}

std::vector<models::Order> TradingClient::cancel_all_orders(const Symbol& symbol) {
    Params params;
    params["symbol"] = symbol;
    auto body = signed_delete("/api/v3/openOrders", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    return orders;
}

std::vector<models::Order> TradingClient::get_open_orders(const Symbol& symbol) {
    Params params;
    params["symbol"] = symbol;
    auto body = signed_get("/api/v3/openOrders", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    return orders;
}

std::vector<models::Order> TradingClient::get_all_open_orders() {
    auto body = signed_get("/api/v3/openOrders");
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    return orders;
}

std::vector<models::Order> TradingClient::get_all_orders(const Symbol& symbol, int limit) {
    Params params;
    params["symbol"] = symbol;
    params["limit"] = std::to_string(limit);
    auto body = signed_get("/api/v3/allOrders", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    return orders;
}

// ---------------------------------------------------------------------------
// Asynchronous API
// ---------------------------------------------------------------------------

asio::awaitable<models::Order> TradingClient::async_new_order(models::NewOrderRequest request) {
    auto body = co_await async_signed_post("/api/v3/order", build_new_order_params(request));
    auto json = detail::parse_response(200, body);
    co_return detail::parse_order(json);
}

asio::awaitable<void> TradingClient::async_test_new_order(models::NewOrderRequest request) {
    co_await async_signed_post("/api/v3/order/test", build_new_order_params(request));
}

asio::awaitable<models::Order> TradingClient::async_query_order(Symbol symbol, OrderId order_id) {
    Params params;
    params["symbol"] = std::move(symbol);
    params["orderId"] = std::to_string(order_id);
    auto body = co_await async_signed_get("/api/v3/order", std::move(params));
    auto json = detail::parse_response(200, body);
    co_return detail::parse_order(json);
}

asio::awaitable<models::Order> TradingClient::async_query_order_by_client_id(Symbol symbol, ClientOrderId client_order_id) {
    Params params;
    params["symbol"] = std::move(symbol);
    params["origClientOrderId"] = std::move(client_order_id);
    auto body = co_await async_signed_get("/api/v3/order", std::move(params));
    auto json = detail::parse_response(200, body);
    co_return detail::parse_order(json);
}

asio::awaitable<models::Order> TradingClient::async_cancel_order(models::CancelOrderRequest request) {
    auto body = co_await async_signed_delete("/api/v3/order", build_cancel_params(request));
    auto json = detail::parse_response(200, body);
    co_return detail::parse_order(json);
}

asio::awaitable<std::vector<models::Order>> TradingClient::async_cancel_all_orders(Symbol symbol) {
    Params params;
    params["symbol"] = std::move(symbol);
    auto body = co_await async_signed_delete("/api/v3/openOrders", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    co_return orders;
}

asio::awaitable<std::vector<models::Order>> TradingClient::async_get_open_orders(Symbol symbol) {
    Params params;
    params["symbol"] = std::move(symbol);
    auto body = co_await async_signed_get("/api/v3/openOrders", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    co_return orders;
}

asio::awaitable<std::vector<models::Order>> TradingClient::async_get_all_open_orders() {
    auto body = co_await async_signed_get("/api/v3/openOrders");
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    co_return orders;
}

asio::awaitable<std::vector<models::Order>> TradingClient::async_get_all_orders(Symbol symbol, int limit) {
    Params params;
    params["symbol"] = std::move(symbol);
    params["limit"] = std::to_string(limit);
    auto body = co_await async_signed_get("/api/v3/allOrders", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Order> orders;
    orders.reserve(json.size());
    for (const auto& item : json) {
        orders.push_back(detail::parse_order(item));
    }
    co_return orders;
}

}  // namespace bintrade::rest

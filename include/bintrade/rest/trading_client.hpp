#pragma once

#include <bintrade/core/export.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/rest/client.hpp>

#include <boost/asio/awaitable.hpp>
#include <vector>

namespace bintrade::rest {

class BINTRADE_API TradingClient : public Client {
public:
    using Client::Client;

    // -----------------------------------------------------------------------
    // Synchronous API
    // -----------------------------------------------------------------------
    [[nodiscard]] models::Order new_order(const models::NewOrderRequest& request);
    void test_new_order(const models::NewOrderRequest& request);

    [[nodiscard]] models::Order query_order(const Symbol& symbol, OrderId order_id);
    [[nodiscard]] models::Order query_order(const Symbol& symbol, const ClientOrderId& client_order_id);

    [[nodiscard]] models::Order cancel_order(const models::CancelOrderRequest& request);
    [[nodiscard]] std::vector<models::Order> cancel_all_orders(const Symbol& symbol);

    [[nodiscard]] std::vector<models::Order> get_open_orders(const Symbol& symbol);
    [[nodiscard]] std::vector<models::Order> get_all_open_orders();
    [[nodiscard]] std::vector<models::Order> get_all_orders(const Symbol& symbol, int limit = 500);

    // -----------------------------------------------------------------------
    // Asynchronous API
    // -----------------------------------------------------------------------
    [[nodiscard]] boost::asio::awaitable<models::Order> async_new_order(models::NewOrderRequest request);
    [[nodiscard]] boost::asio::awaitable<void> async_test_new_order(models::NewOrderRequest request);
    [[nodiscard]] boost::asio::awaitable<models::Order> async_query_order(Symbol symbol, OrderId order_id);
    [[nodiscard]] boost::asio::awaitable<models::Order> async_query_order_by_client_id(Symbol symbol, ClientOrderId client_order_id);
    [[nodiscard]] boost::asio::awaitable<models::Order> async_cancel_order(models::CancelOrderRequest request);
    [[nodiscard]] boost::asio::awaitable<std::vector<models::Order>> async_cancel_all_orders(Symbol symbol);
    [[nodiscard]] boost::asio::awaitable<std::vector<models::Order>> async_get_open_orders(Symbol symbol);
    [[nodiscard]] boost::asio::awaitable<std::vector<models::Order>> async_get_all_open_orders();
    [[nodiscard]] boost::asio::awaitable<std::vector<models::Order>> async_get_all_orders(Symbol symbol, int limit = 500);
};

}  // namespace bintrade::rest

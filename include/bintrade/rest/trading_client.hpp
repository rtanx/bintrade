#pragma once

#include <bintrade/models/order.hpp>
#include <bintrade/rest/client.hpp>

#include <vector>

namespace bintrade::rest {

class TradingClient : public Client {
public:
    using Client::Client;

    [[nodiscard]] models::Order new_order(const models::NewOrderRequest& request);
    void test_new_order(const models::NewOrderRequest& request);

    [[nodiscard]] models::Order query_order(const Symbol& symbol, OrderId order_id);
    [[nodiscard]] models::Order query_order(const Symbol& symbol,
                                            const ClientOrderId& client_order_id);

    [[nodiscard]] models::Order cancel_order(const models::CancelOrderRequest& request);
    [[nodiscard]] std::vector<models::Order> cancel_all_orders(const Symbol& symbol);

    [[nodiscard]] std::vector<models::Order> get_open_orders(const Symbol& symbol);
    [[nodiscard]] std::vector<models::Order> get_all_open_orders();
    [[nodiscard]] std::vector<models::Order> get_all_orders(const Symbol& symbol, int limit = 500);
};

}  // namespace bintrade::rest

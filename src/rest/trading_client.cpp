#include <bintrade/rest/trading_client.hpp>

#include <stdexcept>

namespace bintrade::rest {

models::Order TradingClient::new_order(const models::NewOrderRequest& /*request*/) {
    // TODO: POST /api/v3/order
    throw std::runtime_error("TradingClient::new_order not implemented");
}

void TradingClient::test_new_order(const models::NewOrderRequest& /*request*/) {
    // TODO: POST /api/v3/order/test
    throw std::runtime_error("TradingClient::test_new_order not implemented");
}

models::Order TradingClient::query_order(const Symbol& /*symbol*/, OrderId /*order_id*/) {
    // TODO: GET /api/v3/order
    throw std::runtime_error("TradingClient::query_order not implemented");
}

models::Order TradingClient::query_order(const Symbol& /*symbol*/,
                                         const ClientOrderId& /*client_order_id*/) {
    // TODO: GET /api/v3/order
    throw std::runtime_error("TradingClient::query_order not implemented");
}

models::Order TradingClient::cancel_order(const models::CancelOrderRequest& /*request*/) {
    // TODO: DELETE /api/v3/order
    throw std::runtime_error("TradingClient::cancel_order not implemented");
}

std::vector<models::Order> TradingClient::cancel_all_orders(const Symbol& /*symbol*/) {
    // TODO: DELETE /api/v3/openOrders
    throw std::runtime_error("TradingClient::cancel_all_orders not implemented");
}

std::vector<models::Order> TradingClient::get_open_orders(const Symbol& /*symbol*/) {
    // TODO: GET /api/v3/openOrders
    throw std::runtime_error("TradingClient::get_open_orders not implemented");
}

std::vector<models::Order> TradingClient::get_all_open_orders() {
    // TODO: GET /api/v3/openOrders
    throw std::runtime_error("TradingClient::get_all_open_orders not implemented");
}

std::vector<models::Order> TradingClient::get_all_orders(const Symbol& /*symbol*/, int /*limit*/) {
    // TODO: GET /api/v3/allOrders
    throw std::runtime_error("TradingClient::get_all_orders not implemented");
}

}  // namespace bintrade::rest

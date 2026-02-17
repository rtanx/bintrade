#pragma once

#include <bintrade/core/types.hpp>

#include <optional>

namespace bintrade::models {

struct NewOrderRequest {
    Symbol symbol;
    Side side = Side::Buy;
    OrderType type = OrderType::Limit;
    TimeInForce time_in_force = TimeInForce::GTC;
    Quantity quantity = 0.0;
    std::optional<Price> price;
    std::optional<ClientOrderId> client_order_id;
    std::optional<Price> stop_price;
};

struct Order {
    Symbol symbol;
    OrderId order_id = 0;
    ClientOrderId client_order_id;
    Price price = 0.0;
    Quantity orig_qty = 0.0;
    Quantity executed_qty = 0.0;
    Quantity cummulative_quote_qty = 0.0;
    OrderStatus status = OrderStatus::New;
    TimeInForce time_in_force = TimeInForce::GTC;
    OrderType type = OrderType::Limit;
    Side side = Side::Buy;
    Timestamp time{};
    Timestamp update_time{};
};

struct CancelOrderRequest {
    Symbol symbol;
    std::optional<OrderId> order_id;
    std::optional<ClientOrderId> client_order_id;
};

}  // namespace bintrade::models

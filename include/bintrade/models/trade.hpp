#pragma once

#include <bintrade/core/types.hpp>

#include <cstdint>

namespace bintrade::models {

struct Trade {
    std::uint64_t id = 0;
    OrderId order_id = 0;
    Price price = 0.0;
    Quantity qty = 0.0;
    Quantity quote_qty = 0.0;
    Timestamp time{};
    bool is_buyer = false;
    bool is_maker = false;
};

struct AccountTrade {
    Symbol symbol;
    std::uint64_t id = 0;
    OrderId order_id = 0;
    Price price = 0.0;
    Quantity qty = 0.0;
    Quantity quote_qty = 0.0;
    double commission = 0.0;
    std::string commission_asset;
    Timestamp time{};
    bool is_buyer = false;
    bool is_maker = false;
};

}  // namespace bintrade::models

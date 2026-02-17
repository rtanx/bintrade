#pragma once

#include <bintrade/core/types.hpp>

#include <cstdint>
#include <vector>

namespace bintrade::models {

struct Ticker {
    Symbol symbol;
    Price price = 0.0;
    Timestamp time{};
};

struct Ticker24h {
    Symbol symbol;
    Price open = 0.0;
    Price high = 0.0;
    Price low = 0.0;
    Price close = 0.0;
    Quantity volume = 0.0;
    Quantity quote_volume = 0.0;
    Timestamp open_time{};
    Timestamp close_time{};
    std::uint64_t num_trades = 0;
    Price price_change = 0.0;
    double price_change_percent = 0.0;
};

struct OrderBookEntry {
    Price price = 0.0;
    Quantity quantity = 0.0;
};

struct OrderBook {
    Symbol symbol;
    std::uint64_t last_update_id = 0;
    std::vector<OrderBookEntry> bids;
    std::vector<OrderBookEntry> asks;
};

struct Kline {
    Timestamp open_time{};
    Price open = 0.0;
    Price high = 0.0;
    Price low = 0.0;
    Price close = 0.0;
    Quantity volume = 0.0;
    Timestamp close_time{};
    Quantity quote_volume = 0.0;
    std::uint64_t num_trades = 0;
};

}  // namespace bintrade::models

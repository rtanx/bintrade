#pragma once

// Handler traits and adapter types for typed WebSocket streams.
//
// Users implement a handler struct with some/all of the on_* methods.
// Missing methods are detected via if-constexpr requires-expressions,
// so a handler only needs to implement the events it cares about.

#include <bintrade/models/account.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/models/trade.hpp>

#include <functional>

namespace bintrade::ws {

// -----------------------------------------------------------------------
// FunctionalMarketHandler -- wraps std::function callbacks into the
// handler interface used by TypedMarketStream<Handler>.
// -----------------------------------------------------------------------
struct FunctionalMarketHandler {
    std::function<void(const models::Trade&)> on_trade_fn;
    std::function<void(const models::Kline&)> on_kline_fn;
    std::function<void(const models::Ticker&)> on_ticker_fn;
    std::function<void(const models::OrderBook&)> on_order_book_fn;

    void on_trade(const models::Trade& t) {
        if (on_trade_fn) {
            on_trade_fn(t);
        }
    }
    void on_kline(const models::Kline& k) {
        if (on_kline_fn) {
            on_kline_fn(k);
        }
    }
    void on_ticker(const models::Ticker& t) {
        if (on_ticker_fn) {
            on_ticker_fn(t);
        }
    }
    void on_order_book(const models::OrderBook& ob) {
        if (on_order_book_fn) {
            on_order_book_fn(ob);
        }
    }
};

// -----------------------------------------------------------------------
// FunctionalUserHandler -- wraps std::function callbacks into the
// handler interface used by TypedUserStream<Handler>.
// -----------------------------------------------------------------------
struct FunctionalUserHandler {
    std::function<void(const models::Order&)> on_order_update_fn;
    std::function<void(const models::AccountInfo&)> on_account_update_fn;

    void on_order_update(const models::Order& o) {
        if (on_order_update_fn) {
            on_order_update_fn(o);
        }
    }
    void on_account_update(const models::AccountInfo& a) {
        if (on_account_update_fn) {
            on_account_update_fn(a);
        }
    }
};

}  // namespace bintrade::ws

#pragma once

// WebSocket stream message parsers.
//
// Binance WebSocket stream JSON formats differ from the REST equivalents,
// so these are kept separate from src/rest/detail/json_parse.hpp.
//
// All functions are inline -- this is a header-only parsing utility.

#include <bintrade/core/error.hpp>
#include <bintrade/core/types.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/models/trade.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <string>

namespace bintrade::ws::detail {

// ---------------------------------------------------------------------------
// Trade stream  (<symbol>@trade)
//
// {
//   "e": "trade",
//   "E": 123456789,   -- event time (ms)
//   "s": "BNBBTC",
//   "t": 12345,       -- trade id
//   "p": "0.001",     -- price
//   "q": "100",       -- quantity
//   "T": 123456785,   -- trade time (ms)
//   "m": true         -- is buyer the market maker?
// }
// ---------------------------------------------------------------------------
inline models::Trade parse_trade_stream(const nlohmann::json& j) {
    models::Trade t;
    t.id = j.value("t", uint64_t{0});
    t.price = std::stod(j.value("p", "0"));
    t.qty = std::stod(j.value("q", "0"));
    t.quote_qty = t.price * t.qty;
    t.time = Timestamp(std::chrono::milliseconds(j.value("T", int64_t{0})));
    t.is_maker = j.value("m", false);
    t.is_buyer = !t.is_maker;
    return t;
}

// ---------------------------------------------------------------------------
// Kline stream  (<symbol>@kline_<interval>)
//
// {
//   "e": "kline",
//   "k": {
//     "t": 123400000,  -- open time
//     "T": 123460000,  -- close time
//     "o": "0.0010",
//     "c": "0.0020",
//     "h": "0.0025",
//     "l": "0.0015",
//     "v": "1000",     -- base asset volume
//     "q": "1.0000",   -- quote asset volume
//     "n": 100,        -- number of trades
//     "x": false       -- is this kline closed?
//   }
// }
// ---------------------------------------------------------------------------
inline models::Kline parse_kline_stream(const nlohmann::json& j) {
    const auto& k = j.at("k");
    models::Kline kl;
    kl.open_time = Timestamp(std::chrono::milliseconds(k.value("t", int64_t{0})));
    kl.close_time = Timestamp(std::chrono::milliseconds(k.value("T", int64_t{0})));
    kl.open = std::stod(k.value("o", "0"));
    kl.high = std::stod(k.value("h", "0"));
    kl.low = std::stod(k.value("l", "0"));
    kl.close = std::stod(k.value("c", "0"));
    kl.volume = std::stod(k.value("v", "0"));
    kl.quote_volume = std::stod(k.value("q", "0"));
    kl.num_trades = k.value("n", uint64_t{0});
    return kl;
}

// ---------------------------------------------------------------------------
// Individual symbol ticker stream  (<symbol>@ticker)
//
// {
//   "e": "24hrTicker",
//   "s": "BNBBTC",
//   "c": "0.0025",   -- last price
//   "E": 123456789   -- event time (ms)
// }
// ---------------------------------------------------------------------------
inline models::Ticker parse_ticker_stream(const nlohmann::json& j) {
    models::Ticker t;
    t.symbol = j.value("s", "");
    t.price = std::stod(j.value("c", "0"));
    t.time = Timestamp(std::chrono::milliseconds(j.value("E", int64_t{0})));
    return t;
}

// ---------------------------------------------------------------------------
// Diff depth stream  (<symbol>@depth)
//
// {
//   "e": "depthUpdate",
//   "s": "BNBBTC",
//   "U": 157,          -- first update id
//   "u": 160,          -- final update id
//   "b": [["0.0024","10"],...],
//   "a": [["0.0026","100"],...]
// }
//
// Note: this is a *diff* update, not a full snapshot. The OrderBook returned
// here represents the bids/asks deltas. Callers that maintain a local book
// must apply the update using the U/u sequence numbers themselves.
// The last_update_id field carries the final update id ("u").
// ---------------------------------------------------------------------------
inline models::OrderBook parse_depth_stream(const nlohmann::json& j) {
    models::OrderBook book;
    book.symbol = j.value("s", "");
    book.last_update_id = j.value("u", uint64_t{0});

    const auto parse_level = [](const nlohmann::json& level) {
        models::OrderBookEntry e;
        e.price = std::stod(level[0].get<std::string>());
        e.quantity = std::stod(level[1].get<std::string>());
        return e;
    };

    if (j.contains("b")) {
        book.bids.reserve(j["b"].size());
        for (const auto& bid : j["b"]) {
            book.bids.push_back(parse_level(bid));
        }
    }
    if (j.contains("a")) {
        book.asks.reserve(j["a"].size());
        for (const auto& ask : j["a"]) {
            book.asks.push_back(parse_level(ask));
        }
    }
    return book;
}

// ---------------------------------------------------------------------------
// User data stream -- executionReport (order update)
//
// Relevant fields only -- full spec at:
// https://developers.binance.com/docs/binance-spot-api-docs/user-data-stream
// ---------------------------------------------------------------------------
inline models::Order parse_order_update(const nlohmann::json& j) {
    models::Order o;
    o.symbol = j.value("s", "");
    o.order_id = j.value("i", uint64_t{0});
    o.client_order_id = j.value("c", "");
    o.side = bintrade::side_from_string(j.value("S", "BUY"));
    o.type = bintrade::order_type_from_string(j.value("o", "LIMIT"));
    o.time_in_force = bintrade::time_in_force_from_string(j.value("f", "GTC"));
    o.orig_qty = std::stod(j.value("q", "0"));
    o.price = std::stod(j.value("p", "0"));
    o.executed_qty = std::stod(j.value("z", "0"));
    o.cummulative_quote_qty = std::stod(j.value("Z", "0"));
    o.status = bintrade::order_status_from_string(j.value("X", "NEW"));
    o.time = Timestamp(std::chrono::milliseconds(j.value("O", int64_t{0})));
    o.update_time = Timestamp(std::chrono::milliseconds(j.value("T", int64_t{0})));
    return o;
}

// ---------------------------------------------------------------------------
// User data stream -- outboundAccountPosition (account update)
//
// {
//   "e": "outboundAccountPosition",
//   "E": 1564034571105,
//   "u": 1564034571073,
//   "B": [
//     { "a": "ETH", "f": "10000.000000", "l": "0.000000" }
//   ]
// }
// ---------------------------------------------------------------------------
inline models::AccountInfo parse_account_update(const nlohmann::json& j) {
    models::AccountInfo info;
    info.update_time = Timestamp(std::chrono::milliseconds(j.value("u", int64_t{0})));
    if (j.contains("B")) {
        for (const auto& b : j["B"]) {
            models::Balance bal;
            bal.asset = b.value("a", "");
            bal.free = std::stod(b.value("f", "0"));
            bal.locked = std::stod(b.value("l", "0"));
            info.balances.push_back(std::move(bal));
        }
    }
    return info;
}

}  // namespace bintrade::ws::detail

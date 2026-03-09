#pragma once

#include <bintrade/core/error.hpp>
#include <bintrade/core/types.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/models/trade.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <utility>
#include <vector>

namespace bintrade::rest::detail {

/// Throw an ApiException if the response body contains a Binance error.
inline void check_api_error(int status_code, const nlohmann::json& json) {
    if (status_code >= 400 && json.contains("code") && json.contains("msg")) {
        throw ApiException(json["code"].get<int>(), json["msg"].get<std::string>());
    }
}

/// Parse the JSON response body, throwing on API errors.
inline nlohmann::json parse_response(int status_code, const std::string& body) {
    if (body.empty()) {
        if (status_code >= 400) {
            throw ApiException(status_code, "Empty error response");
        }
        return {};
    }
    auto json = nlohmann::json::parse(body);
    check_api_error(status_code, json);
    return json;
}

// ---------------------------------------------------------------------------
// Model parsers — from Binance JSON to bintrade:: types
// ---------------------------------------------------------------------------

inline models::Ticker parse_ticker(const nlohmann::json& json) {
    models::Ticker t;
    t.symbol = json.value("symbol", "");
    t.price = std::stod(json.value("price", "0"));
    return t;
}

inline models::Ticker24h parse_ticker_24h(const nlohmann::json& json) {
    models::Ticker24h t;
    t.symbol = json.value("symbol", "");
    t.price_change = std::stod(json.value("priceChange", "0"));
    t.price_change_percent = std::stod(json.value("priceChangePercent", "0"));
    // Ticker24h has open/high/low/close instead of separate last/bid/ask
    t.open = std::stod(json.value("openPrice", "0"));
    t.high = std::stod(json.value("highPrice", "0"));
    t.low = std::stod(json.value("lowPrice", "0"));
    t.close = std::stod(json.value("lastPrice", "0"));
    t.volume = std::stod(json.value("volume", "0"));
    t.quote_volume = std::stod(json.value("quoteVolume", "0"));
    t.open_time = Timestamp(std::chrono::milliseconds(json.value("openTime", int64_t{0})));
    t.close_time = Timestamp(std::chrono::milliseconds(json.value("closeTime", int64_t{0})));
    t.num_trades = json.value("count", uint64_t{0});
    return t;
}

inline models::OrderBookEntry parse_order_book_entry(const nlohmann::json& json) {
    models::OrderBookEntry e;
    e.price = std::stod(json[0].get<std::string>());
    e.quantity = std::stod(json[1].get<std::string>());
    return e;
}

inline models::OrderBook parse_order_book(const nlohmann::json& json, const std::string& symbol) {
    models::OrderBook book;
    book.symbol = symbol;
    book.last_update_id = json.value("lastUpdateId", uint64_t{0});
    for (const auto& bid : json["bids"]) {
        book.bids.push_back(parse_order_book_entry(bid));
    }
    for (const auto& ask : json["asks"]) {
        book.asks.push_back(parse_order_book_entry(ask));
    }
    return book;
}

inline models::Trade parse_trade(const nlohmann::json& json) {
    models::Trade t;
    t.id = json.value("id", uint64_t{0});
    t.price = std::stod(json.value("price", "0"));
    t.qty = std::stod(json.value("qty", "0"));
    t.quote_qty = std::stod(json.value("quoteQty", "0"));
    t.time = Timestamp(std::chrono::milliseconds(json.value("time", int64_t{0})));
    t.is_buyer = json.value("isBuyerMaker", false);
    t.is_maker = json.value("isBuyerMaker", false);
    return t;
}

inline models::Kline parse_kline(const nlohmann::json& json) {
    models::Kline k;
    k.open_time = Timestamp(std::chrono::milliseconds(json[0].get<int64_t>()));
    k.open = std::stod(json[1].get<std::string>());
    k.high = std::stod(json[2].get<std::string>());
    k.low = std::stod(json[3].get<std::string>());
    k.close = std::stod(json[4].get<std::string>());
    k.volume = std::stod(json[5].get<std::string>());
    k.close_time = Timestamp(std::chrono::milliseconds(json[6].get<int64_t>()));
    k.quote_volume = std::stod(json[7].get<std::string>());
    k.num_trades = json[8].get<uint64_t>();
    return k;
}

inline models::Order parse_order(const nlohmann::json& json) {
    models::Order o;
    o.symbol = json.value("symbol", "");
    o.order_id = json.value("orderId", uint64_t{0});
    o.client_order_id = json.value("clientOrderId", "");
    o.price = std::stod(json.value("price", "0"));
    o.orig_qty = std::stod(json.value("origQty", "0"));
    o.executed_qty = std::stod(json.value("executedQty", "0"));
    o.cummulative_quote_qty = std::stod(json.value("cummulativeQuoteQty", "0"));
    o.status = bintrade::order_status_from_string(json.value("status", "NEW"));
    o.type = bintrade::order_type_from_string(json.value("type", "LIMIT"));
    o.side = bintrade::side_from_string(json.value("side", "BUY"));
    o.time_in_force = bintrade::time_in_force_from_string(json.value("timeInForce", "GTC"));
    o.time = Timestamp(std::chrono::milliseconds(json.value("time", int64_t{0})));
    o.update_time = Timestamp(std::chrono::milliseconds(json.value("updateTime", int64_t{0})));
    return o;
}

inline models::Balance parse_balance(const nlohmann::json& json) {
    models::Balance b;
    b.asset = json.value("asset", "");
    b.free = std::stod(json.value("free", "0"));
    b.locked = std::stod(json.value("locked", "0"));
    return b;
}

inline models::AccountInfo parse_account_info(const nlohmann::json& json) {
    models::AccountInfo info;
    info.can_trade = json.value("canTrade", false);
    info.can_withdraw = json.value("canWithdraw", false);
    info.can_deposit = json.value("canDeposit", false);
    info.account_type = json.value("accountType", "");
    info.update_time = Timestamp(std::chrono::milliseconds(json.value("updateTime", int64_t{0})));
    for (const auto& b : json["balances"]) {
        auto balance = parse_balance(b);
        if (balance.free > 0.0 || balance.locked > 0.0) {
            info.balances.push_back(std::move(balance));
        }
    }
    return info;
}

inline models::AccountTrade parse_account_trade(const nlohmann::json& json) {
    models::AccountTrade t;
    t.symbol = json.value("symbol", "");
    t.id = json.value("id", uint64_t{0});
    t.order_id = json.value("orderId", uint64_t{0});
    t.price = std::stod(json.value("price", "0"));
    t.qty = std::stod(json.value("qty", "0"));
    t.quote_qty = std::stod(json.value("quoteQty", "0"));
    t.commission = std::stod(json.value("commission", "0"));
    t.commission_asset = json.value("commissionAsset", "");
    t.time = Timestamp(std::chrono::milliseconds(json.value("time", int64_t{0})));
    t.is_buyer = json.value("isBuyer", false);
    t.is_maker = json.value("isMaker", false);
    return t;
}

}  // namespace bintrade::rest::detail

#include "detail/json_parse.hpp"

#include <bintrade/rest/market_data_client.hpp>

#include <nlohmann/json.hpp>

#include <boost/asio/awaitable.hpp>
#include <string>
#include <vector>

namespace bintrade::rest {

namespace asio = boost::asio;

// ---------------------------------------------------------------------------
// Synchronous API
// ---------------------------------------------------------------------------

models::OrderBook MarketDataClient::get_order_book(const Symbol& symbol, int limit) {
    Params params;
    params["symbol"] = symbol;
    params["limit"] = std::to_string(limit);
    auto body = public_get("/api/v3/depth", params);
    auto json = detail::parse_response(200, body);
    return detail::parse_order_book(json, symbol);
}

std::vector<models::Trade> MarketDataClient::get_recent_trades(const Symbol& symbol, int limit) {
    Params params;
    params["symbol"] = symbol;
    params["limit"] = std::to_string(limit);
    auto body = public_get("/api/v3/trades", params);
    auto json = detail::parse_response(200, body);
    std::vector<models::Trade> trades;
    trades.reserve(json.size());
    for (const auto& item : json) {
        trades.push_back(detail::parse_trade(item));
    }
    return trades;
}

std::vector<models::Kline> MarketDataClient::get_klines(const Symbol& symbol, std::string_view interval, int limit) {
    Params params;
    params["symbol"] = symbol;
    params["interval"] = std::string(interval);
    params["limit"] = std::to_string(limit);
    auto body = public_get("/api/v3/klines", params);
    auto json = detail::parse_response(200, body);
    std::vector<models::Kline> klines;
    klines.reserve(json.size());
    for (const auto& item : json) {
        klines.push_back(detail::parse_kline(item));
    }
    return klines;
}

models::Ticker24h MarketDataClient::get_ticker_24h(const Symbol& symbol) {
    Params params;
    params["symbol"] = symbol;
    auto body = public_get("/api/v3/ticker/24hr", params);
    auto json = detail::parse_response(200, body);
    return detail::parse_ticker_24h(json);
}

models::Ticker MarketDataClient::get_price_ticker(const Symbol& symbol) {
    Params params;
    params["symbol"] = symbol;
    auto body = public_get("/api/v3/ticker/price", params);
    auto json = detail::parse_response(200, body);
    return detail::parse_ticker(json);
}

std::vector<models::Ticker> MarketDataClient::get_all_price_tickers() {
    auto body = public_get("/api/v3/ticker/price");
    auto json = detail::parse_response(200, body);
    std::vector<models::Ticker> tickers;
    tickers.reserve(json.size());
    for (const auto& item : json) {
        tickers.push_back(detail::parse_ticker(item));
    }
    return tickers;
}

// ---------------------------------------------------------------------------
// Asynchronous API
// ---------------------------------------------------------------------------

asio::awaitable<models::OrderBook> MarketDataClient::async_get_order_book(Symbol symbol, int limit) {
    Params params;
    params["symbol"] = symbol;  // copy before potential move
    params["limit"] = std::to_string(limit);
    auto body = co_await async_public_get("/api/v3/depth", std::move(params));
    auto json = detail::parse_response(200, body);
    co_return detail::parse_order_book(json, symbol);
}

asio::awaitable<std::vector<models::Trade>> MarketDataClient::async_get_recent_trades(Symbol symbol, int limit) {
    Params params;
    params["symbol"] = std::move(symbol);
    params["limit"] = std::to_string(limit);
    auto body = co_await async_public_get("/api/v3/trades", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Trade> trades;
    trades.reserve(json.size());
    for (const auto& item : json) {
        trades.push_back(detail::parse_trade(item));
    }
    co_return trades;
}

asio::awaitable<std::vector<models::Kline>> MarketDataClient::async_get_klines(Symbol symbol, std::string interval, int limit) {
    Params params;
    params["symbol"] = std::move(symbol);
    params["interval"] = std::move(interval);
    params["limit"] = std::to_string(limit);
    auto body = co_await async_public_get("/api/v3/klines", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::Kline> klines;
    klines.reserve(json.size());
    for (const auto& item : json) {
        klines.push_back(detail::parse_kline(item));
    }
    co_return klines;
}

asio::awaitable<models::Ticker24h> MarketDataClient::async_get_ticker_24h(Symbol symbol) {
    Params params;
    params["symbol"] = std::move(symbol);
    auto body = co_await async_public_get("/api/v3/ticker/24hr", std::move(params));
    auto json = detail::parse_response(200, body);
    co_return detail::parse_ticker_24h(json);
}

asio::awaitable<models::Ticker> MarketDataClient::async_get_price_ticker(Symbol symbol) {
    Params params;
    params["symbol"] = std::move(symbol);
    auto body = co_await async_public_get("/api/v3/ticker/price", std::move(params));
    auto json = detail::parse_response(200, body);
    co_return detail::parse_ticker(json);
}

asio::awaitable<std::vector<models::Ticker>> MarketDataClient::async_get_all_price_tickers() {
    auto body = co_await async_public_get("/api/v3/ticker/price");
    auto json = detail::parse_response(200, body);
    std::vector<models::Ticker> tickers;
    tickers.reserve(json.size());
    for (const auto& item : json) {
        tickers.push_back(detail::parse_ticker(item));
    }
    co_return tickers;
}

}  // namespace bintrade::rest

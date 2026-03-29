#pragma once

#include <bintrade/core/export.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/rest/client.hpp>

#include <boost/asio/awaitable.hpp>
#include <string_view>
#include <vector>

namespace bintrade::rest {

class BINTRADE_API MarketDataClient : public Client {
public:
    using Client::Client;

    // -----------------------------------------------------------------------
    // Synchronous API
    // -----------------------------------------------------------------------
    [[nodiscard]] models::OrderBook get_order_book(const Symbol& symbol, int limit = 100);
    [[nodiscard]] std::vector<models::Trade> get_recent_trades(const Symbol& symbol, int limit = 500);
    [[nodiscard]] std::vector<models::Kline> get_klines(const Symbol& symbol, std::string_view interval, int limit = 500);
    [[nodiscard]] models::Ticker24h get_ticker_24h(const Symbol& symbol);
    [[nodiscard]] models::Ticker get_price_ticker(const Symbol& symbol);
    [[nodiscard]] std::vector<models::Ticker> get_all_price_tickers();

    // -----------------------------------------------------------------------
    // Asynchronous API
    // -----------------------------------------------------------------------
    [[nodiscard]] boost::asio::awaitable<models::OrderBook> async_get_order_book(Symbol symbol, int limit = 100);
    [[nodiscard]] boost::asio::awaitable<std::vector<models::Trade>> async_get_recent_trades(Symbol symbol, int limit = 500);
    [[nodiscard]] boost::asio::awaitable<std::vector<models::Kline>> async_get_klines(Symbol symbol, std::string interval, int limit = 500);
    [[nodiscard]] boost::asio::awaitable<models::Ticker24h> async_get_ticker_24h(Symbol symbol);
    [[nodiscard]] boost::asio::awaitable<models::Ticker> async_get_price_ticker(Symbol symbol);
    [[nodiscard]] boost::asio::awaitable<std::vector<models::Ticker>> async_get_all_price_tickers();
};

}  // namespace bintrade::rest

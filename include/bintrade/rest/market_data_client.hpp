#pragma once

#include <bintrade/models/market_data.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/rest/client.hpp>

#include <string_view>
#include <vector>

namespace bintrade::rest {

class MarketDataClient : public Client {
public:
    using Client::Client;

    [[nodiscard]] models::OrderBook get_order_book(const Symbol& symbol, int limit = 100);
    [[nodiscard]] std::vector<models::Trade> get_recent_trades(const Symbol& symbol, int limit = 500);
    [[nodiscard]] std::vector<models::Kline> get_klines(const Symbol& symbol, std::string_view interval, int limit = 500);
    [[nodiscard]] models::Ticker24h get_ticker_24h(const Symbol& symbol);
    [[nodiscard]] models::Ticker get_price_ticker(const Symbol& symbol);
    [[nodiscard]] std::vector<models::Ticker> get_all_price_tickers();
};

}  // namespace bintrade::rest

#pragma once

#include <bintrade/core/types.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/ws/client.hpp>

#include <functional>

namespace bintrade::ws {

using TradeCallback = std::function<void(const models::Trade& trade)>;
using KlineCallback = std::function<void(const models::Kline& kline)>;
using TickerCallback = std::function<void(const models::Ticker& ticker)>;
using OrderBookCallback = std::function<void(const models::OrderBook& orderbook)>;

class MarketStream : public Client {
public:
    using Client::Client;

    void subscribe_trades(const Symbol& symbol, const TradeCallback& callback);
    void subscribe_klines(const Symbol& symbol, std::string_view interval, const KlineCallback& callback);
    void subscribe_ticker(const Symbol& symbol, const TickerCallback& callback);
    void subscribe_depth(const Symbol& symbol, const OrderBookCallback& callback);
};

}  // namespace bintrade::ws

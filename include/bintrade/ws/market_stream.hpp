#pragma once

#include <bintrade/core/config.hpp>
#include <bintrade/core/export.hpp>
#include <bintrade/core/types.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/ws/client.hpp>

#include <functional>
#include <memory>

namespace bintrade::ws {

using TradeCallback = std::function<void(const models::Trade& trade)>;
using KlineCallback = std::function<void(const models::Kline& kline)>;
using TickerCallback = std::function<void(const models::Ticker& ticker)>;
using OrderBookCallback = std::function<void(const models::OrderBook& orderbook)>;

class BINTRADE_API MarketStream : public Client {
public:
    explicit MarketStream(WebSocketConfig config = WebSocketConfig{});
    ~MarketStream() override;
    MarketStream(const MarketStream&) = delete;
    MarketStream& operator=(const MarketStream&) = delete;
    MarketStream(MarketStream&&) noexcept;
    MarketStream& operator=(MarketStream&&) noexcept;

    // Set the dispatch mode. Must be called before any subscribe_* method.
    // Default is DispatchMode::Inline (current behaviour).
    void set_dispatch_mode(DispatchMode mode);

    void subscribe_trades(const Symbol& symbol, const TradeCallback& callback);
    void subscribe_klines(const Symbol& symbol, std::string_view interval, const KlineCallback& callback);
    void subscribe_ticker(const Symbol& symbol, const TickerCallback& callback);
    void subscribe_depth(const Symbol& symbol, const OrderBookCallback& callback);

private:
    DispatchMode dispatch_mode_ = DispatchMode::Inline;
    bool subscribed_ = false;  // Guards set_dispatch_mode after subscribe.

    struct QueueState;
    std::unique_ptr<QueueState> queue_state_;

    void install_dispatch(std::function<void(std::string_view)> dispatch_fn);
    void start_consumer(std::function<void(std::string_view)> dispatch_fn);
    void stop_consumer();
};

}  // namespace bintrade::ws

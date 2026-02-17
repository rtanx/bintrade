#include <bintrade/ws/market_stream.hpp>

#include <stdexcept>

namespace bintrade::ws {

void MarketStream::subscribe_trades(const Symbol& /*symbol*/, const TradeCallback& /*callback*/) {
    // TODO: Subscribe to <symbol>@trade stream
    throw std::runtime_error("MarketStream::subscribe_trades not implemented");
}

void MarketStream::subscribe_klines(const Symbol& /*symbol*/, std::string_view /*interval*/,
                                    const KlineCallback& /*callback*/) {
    // TODO: Subscribe to <symbol>@kline_<interval> stream
    throw std::runtime_error("MarketStream::subscribe_klines not implemented");
}

void MarketStream::subscribe_ticker(const Symbol& /*symbol*/, const TickerCallback& /*callback*/) {
    // TODO: Subscribe to <symbol>@ticker stream
    throw std::runtime_error("MarketStream::subscribe_ticker not implemented");
}

void MarketStream::subscribe_depth(const Symbol& /*symbol*/,
                                   const OrderBookCallback& /*callback*/) {
    // TODO: Subscribe to <symbol>@depth stream
    throw std::runtime_error("MarketStream::subscribe_depth not implemented");
}

}  // namespace bintrade::ws

#include <bintrade/rest/market_data_client.hpp>

#include <stdexcept>

namespace bintrade::rest {

models::OrderBook MarketDataClient::get_order_book(const Symbol& /*symbol*/, int /*limit*/) {
    // TODO: GET /api/v3/depth
    throw std::runtime_error("MarketDataClient::get_order_book not implemented");
}

std::vector<models::Trade> MarketDataClient::get_recent_trades(const Symbol& /*symbol*/,
                                                               int /*limit*/) {
    // TODO: GET /api/v3/trades
    throw std::runtime_error("MarketDataClient::get_recent_trades not implemented");
}

std::vector<models::Kline> MarketDataClient::get_klines(const Symbol& /*symbol*/,
                                                        std::string_view /*interval*/,
                                                        int /*limit*/) {
    // TODO: GET /api/v3/klines
    throw std::runtime_error("MarketDataClient::get_klines not implemented");
}

models::Ticker24h MarketDataClient::get_ticker_24h(const Symbol& /*symbol*/) {
    // TODO: GET /api/v3/ticker/24hr
    throw std::runtime_error("MarketDataClient::get_ticker_24h not implemented");
}

models::Ticker MarketDataClient::get_price_ticker(const Symbol& /*symbol*/) {
    // TODO: GET /api/v3/ticker/price
    throw std::runtime_error("MarketDataClient::get_price_ticker not implemented");
}

std::vector<models::Ticker> MarketDataClient::get_all_price_tickers() {
    // TODO: GET /api/v3/ticker/price
    throw std::runtime_error("MarketDataClient::get_all_price_tickers not implemented");
}

}  // namespace bintrade::rest

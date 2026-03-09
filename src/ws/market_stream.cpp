#include "detail/ws_parse.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/ws/market_stream.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>

namespace bintrade::ws {

namespace {

// Binance stream names are lower-case symbol + event suffix.
std::string lower(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

void MarketStream::subscribe_trades(const Symbol& symbol, const TradeCallback& callback) {
    auto stream_name = lower(symbol) + "@trade";

    set_message_callback([callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "trade") {
                callback(detail::parse_trade_stream(j));
            }
        } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
            // Discard malformed messages -- the error handler is reserved for
            // connection-level errors. Parsing failures here are non-fatal.
        }
    });

    connect("/ws/" + stream_name);
}

void MarketStream::subscribe_klines(const Symbol& symbol, std::string_view interval, const KlineCallback& callback) {
    auto stream_name = lower(symbol) + "@kline_" + std::string(interval);

    set_message_callback([callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "kline") {
                callback(detail::parse_kline_stream(j));
            }
        } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
        }
    });

    connect("/ws/" + stream_name);
}

void MarketStream::subscribe_ticker(const Symbol& symbol, const TickerCallback& callback) {
    auto stream_name = lower(symbol) + "@ticker";

    set_message_callback([callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "24hrTicker") {
                callback(detail::parse_ticker_stream(j));
            }
        } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
        }
    });

    connect("/ws/" + stream_name);
}

void MarketStream::subscribe_depth(const Symbol& symbol, const OrderBookCallback& callback) {
    auto stream_name = lower(symbol) + "@depth";

    set_message_callback([callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "depthUpdate") {
                callback(detail::parse_depth_stream(j));
            }
        } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
        }
    });

    connect("/ws/" + stream_name);
}

}  // namespace bintrade::ws

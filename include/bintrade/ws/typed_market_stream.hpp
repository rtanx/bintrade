#pragma once

// TypedMarketStream<Handler> -- high-performance market stream with
// compile-time callback dispatch.
//
// The Handler type provides one or more of:
//   void on_trade(const models::Trade&)
//   void on_kline(const models::Kline&)
//   void on_ticker(const models::Ticker&)
//   void on_order_book(const models::OrderBook&)
//
// Only methods present on the handler are compiled; missing methods
// are silently skipped via if-constexpr.
//
// Example:
//   struct MyHandler {
//       void on_trade(const bintrade::models::Trade& t) noexcept { ... }
//   };
//   MyHandler h;
//   bintrade::ws::TypedMarketStream<MyHandler> stream(h);
//   stream.subscribe_trades("BTCUSDT");

#include <bintrade/core/types.hpp>
#include <bintrade/ws/client.hpp>
#include <bintrade/ws/detail/ws_parse.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace bintrade::ws {

template <typename Handler>
class TypedMarketStream : public Client {
public:
    explicit TypedMarketStream(Handler& handler, WebSocketConfig config = WebSocketConfig{}) : Client(std::move(config)), handler_(handler) {}

    void subscribe_trades(const Symbol& symbol) {
        auto stream_name = to_lower(symbol) + "@trade";

        set_message_callback([this](std::string_view msg) {
            try {
                auto j = nlohmann::json::parse(msg);
                if (j.value("e", "") == "trade") {
                    if constexpr (requires { handler_.on_trade(std::declval<const models::Trade&>()); }) {
                        handler_.on_trade(detail::parse_trade_stream(j));
                    }
                }
            } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
            }
        });

        connect("/ws/" + stream_name);
    }

    void subscribe_klines(const Symbol& symbol, std::string_view interval) {
        auto stream_name = to_lower(symbol) + "@kline_" + std::string(interval);

        set_message_callback([this](std::string_view msg) {
            try {
                auto j = nlohmann::json::parse(msg);
                if (j.value("e", "") == "kline") {
                    if constexpr (requires { handler_.on_kline(std::declval<const models::Kline&>()); }) {
                        handler_.on_kline(detail::parse_kline_stream(j));
                    }
                }
            } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
            }
        });

        connect("/ws/" + stream_name);
    }

    void subscribe_ticker(const Symbol& symbol) {
        auto stream_name = to_lower(symbol) + "@ticker";

        set_message_callback([this](std::string_view msg) {
            try {
                auto j = nlohmann::json::parse(msg);
                if (j.value("e", "") == "24hrTicker") {
                    if constexpr (requires { handler_.on_ticker(std::declval<const models::Ticker&>()); }) {
                        handler_.on_ticker(detail::parse_ticker_stream(j));
                    }
                }
            } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
            }
        });

        connect("/ws/" + stream_name);
    }

    void subscribe_depth(const Symbol& symbol) {
        auto stream_name = to_lower(symbol) + "@depth";

        set_message_callback([this](std::string_view msg) {
            try {
                auto j = nlohmann::json::parse(msg);
                if (j.value("e", "") == "depthUpdate") {
                    if constexpr (requires { handler_.on_order_book(std::declval<const models::OrderBook&>()); }) {
                        handler_.on_order_book(detail::parse_depth_stream(j));
                    }
                }
            } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
            }
        });

        connect("/ws/" + stream_name);
    }

private:
    Handler& handler_;

    static std::string to_lower(std::string s) {
        std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }
};

}  // namespace bintrade::ws

#pragma once

#include <string_view>

// Realistic Binance REST API and WebSocket JSON response fixtures.
//
// Each constant is a constexpr string_view containing the minimal JSON payload
// that the corresponding parse function in json_parse.hpp / ws_parse.hpp
// requires. Values are deterministic and chosen to be easy to assert against
// in unit tests (round numbers, recognisable timestamps, etc.).

namespace bintrade::test::fixtures {

// ---- REST: Market Data ----

inline constexpr std::string_view ping_response = "{}";

inline constexpr std::string_view server_time_response = R"({"serverTime":1704067200000})";

inline constexpr std::string_view ticker_response = R"({"symbol":"BTCUSDT","price":"42000.50"})";

inline constexpr std::string_view ticker24h_response = R"({
  "symbol":"BTCUSDT",
  "priceChange":"1500.00",
  "priceChangePercent":"3.70",
  "openPrice":"40500.00",
  "highPrice":"43000.00",
  "lowPrice":"40000.00",
  "lastPrice":"42000.50",
  "volume":"12345.678",
  "quoteVolume":"518400000.00",
  "openTime":1704067200000,
  "closeTime":1704153600000,
  "count":987654
})";

inline constexpr std::string_view order_book_response = R"({
  "lastUpdateId":12345678,
  "bids":[["42000.00","1.500"],["41999.00","2.300"]],
  "asks":[["42001.00","0.750"],["42002.00","1.200"]]
})";

inline constexpr std::string_view recent_trades_response = R"([{
  "id":100000001,
  "price":"42000.50",
  "qty":"0.123",
  "quoteQty":"5166.06",
  "time":1704067200000,
  "isBuyerMaker":false
}])";

inline constexpr std::string_view klines_response = R"([
  [1704067200000,"42000.00","43000.00","41000.00","42500.00","100.5",1704153600000,"4221750.00",500,"50.25","2110875.00","0"]
])";

// ---- REST: Trading ----

inline constexpr std::string_view new_order_response = R"({
  "symbol":"BTCUSDT",
  "orderId":123456789,
  "clientOrderId":"myOrder1",
  "price":"42000.00",
  "origQty":"1.000",
  "executedQty":"0.000",
  "cummulativeQuoteQty":"0.00",
  "status":"NEW",
  "type":"LIMIT",
  "side":"BUY",
  "timeInForce":"GTC",
  "time":1704067200000,
  "updateTime":1704067200000
})";

inline constexpr std::string_view filled_order_response = R"({
  "symbol":"BTCUSDT",
  "orderId":123456789,
  "clientOrderId":"myOrder1",
  "price":"42000.00",
  "origQty":"1.000",
  "executedQty":"1.000",
  "cummulativeQuoteQty":"42000.00",
  "status":"FILLED",
  "type":"LIMIT",
  "side":"BUY",
  "timeInForce":"GTC",
  "time":1704067200000,
  "updateTime":1704067201000
})";

inline constexpr std::string_view cancel_order_response = R"({
  "symbol":"BTCUSDT",
  "orderId":123456789,
  "clientOrderId":"myOrder1",
  "price":"42000.00",
  "origQty":"1.000",
  "executedQty":"0.000",
  "cummulativeQuoteQty":"0.00",
  "status":"CANCELED",
  "type":"LIMIT",
  "side":"BUY",
  "timeInForce":"GTC",
  "time":1704067200000,
  "updateTime":1704067202000
})";

inline constexpr std::string_view open_orders_response = R"([{
  "symbol":"BTCUSDT",
  "orderId":123456789,
  "clientOrderId":"myOrder1",
  "price":"42000.00",
  "origQty":"1.000",
  "executedQty":"0.000",
  "cummulativeQuoteQty":"0.00",
  "status":"NEW",
  "type":"LIMIT",
  "side":"BUY",
  "timeInForce":"GTC",
  "time":1704067200000,
  "updateTime":1704067200000
}])";

// ---- REST: Account ----

inline constexpr std::string_view account_info_response = R"({
  "canTrade":true,
  "canWithdraw":true,
  "canDeposit":true,
  "accountType":"SPOT",
  "updateTime":1704067200000,
  "balances":[
    {"asset":"BTC","free":"1.50000000","locked":"0.50000000"},
    {"asset":"USDT","free":"50000.00","locked":"10000.00"},
    {"asset":"ETH","free":"0.00000000","locked":"0.00000000"}
  ]
})";

inline constexpr std::string_view account_trades_response = R"([{
  "symbol":"BTCUSDT",
  "id":200000001,
  "orderId":123456789,
  "price":"42000.00",
  "qty":"1.000",
  "quoteQty":"42000.00",
  "commission":"0.001",
  "commissionAsset":"BTC",
  "time":1704067200000,
  "isBuyer":true,
  "isMaker":false
}])";

// ---- REST: Error ----

inline constexpr std::string_view api_error_response = R"({"code":-1121,"msg":"Invalid symbol."})";

inline constexpr std::string_view rate_limit_error_response = R"({"code":-1015,"msg":"Too many requests."})";

// ---- WebSocket: Market Streams ----

inline constexpr std::string_view ws_trade_stream = R"({
  "e":"trade",
  "E":1704067200123,
  "s":"BTCUSDT",
  "t":100000001,
  "p":"42000.50",
  "q":"0.123",
  "b":200000001,
  "a":200000002,
  "T":1704067200100,
  "m":false,
  "M":true
})";

inline constexpr std::string_view ws_kline_stream = R"({
  "e":"kline",
  "E":1704067200123,
  "s":"BTCUSDT",
  "k":{
    "t":1704067200000,
    "T":1704153600000,
    "s":"BTCUSDT",
    "i":"1h",
    "o":"42000.00",
    "c":"42500.00",
    "h":"43000.00",
    "l":"41000.00",
    "v":"100.5",
    "q":"4221750.00",
    "n":500,
    "x":false
  }
})";

inline constexpr std::string_view ws_ticker_stream = R"({
  "e":"24hrTicker",
  "E":1704067200123,
  "s":"BTCUSDT",
  "p":"1500.00",
  "P":"3.70",
  "o":"40500.00",
  "h":"43000.00",
  "l":"40000.00",
  "c":"42000.50",
  "v":"12345.678",
  "q":"518400000.00",
  "O":1704067200000,
  "C":1704153600000,
  "n":987654
})";

inline constexpr std::string_view ws_depth_stream = R"({
  "e":"depthUpdate",
  "E":1704067200123,
  "s":"BTCUSDT",
  "U":12345670,
  "u":12345678,
  "b":[["42000.00","1.500"],["41999.00","2.300"]],
  "a":[["42001.00","0.750"],["42002.00","1.200"]]
})";

// ---- WebSocket: User Data Streams ----

inline constexpr std::string_view ws_order_update = R"({
  "e":"executionReport",
  "E":1704067200123,
  "s":"BTCUSDT",
  "S":"BUY",
  "o":"LIMIT",
  "f":"GTC",
  "q":"1.000",
  "p":"42000.00",
  "x":"NEW",
  "X":"NEW",
  "i":123456789,
  "l":"0.000",
  "z":"0.000",
  "L":"0.00",
  "n":"0.00",
  "N":"BTC",
  "T":1704067200100,
  "t":0,
  "O":1704067200000,
  "Z":"0.00"
})";

inline constexpr std::string_view ws_account_update = R"({
  "e":"outboundAccountPosition",
  "E":1704067200123,
  "u":1704067200100,
  "B":[
    {"a":"BTC","f":"1.50000000","l":"0.50000000"},
    {"a":"USDT","f":"50000.00","l":"10000.00"}
  ]
})";

}  // namespace bintrade::test::fixtures

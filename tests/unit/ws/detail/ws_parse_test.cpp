#include "ws/detail/ws_parse.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/core/types.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/models/trade.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace bintrade::test {

using nlohmann::json;
using namespace bintrade::ws::detail;

// ---------------------------------------------------------------------------
// parse_trade_stream  (<symbol>@trade)
// Field differences vs REST: "t" = trade id, "T" = trade time, "m" = maker flag
// ---------------------------------------------------------------------------

TEST(WsParseTradeStreamTest, AllFields) {
    const char* body = R"({
        "e": "trade",
        "E": 123456789,
        "s": "BNBBTC",
        "t": 12345,
        "p": "0.001",
        "q": "100",
        "T": 123456785,
        "m": true
    })";
    json j = json::parse(body);
    auto trade = parse_trade_stream(j);

    EXPECT_EQ(trade.id, 12'345U);
    EXPECT_NEAR(trade.price, 0.001, 1e-9);
    EXPECT_NEAR(trade.qty, 100.0, 1e-6);
    // quote_qty computed as price * qty
    EXPECT_NEAR(trade.quote_qty, 0.001 * 100.0, 1e-9);
    EXPECT_TRUE(trade.is_maker);
    EXPECT_FALSE(trade.is_buyer);  // is_buyer = !is_maker when maker=true

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(trade.time.time_since_epoch()).count(), 123'456'785LL);
}

TEST(WsParseTradeStreamTest, NonMakerSide) {
    json j = json::parse(R"({"e":"trade","t":1,"p":"50000","q":"0.01","T":0,"m":false})");
    auto trade = parse_trade_stream(j);

    EXPECT_FALSE(trade.is_maker);
    EXPECT_TRUE(trade.is_buyer);
}

TEST(WsParseTradeStreamTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto trade = parse_trade_stream(j);

    EXPECT_EQ(trade.id, 0U);
    EXPECT_DOUBLE_EQ(trade.price, 0.0);
    EXPECT_FALSE(trade.is_maker);
}

TEST(WsParseTradeStreamTest, QuoteQtyIsComputedNotNested) {
    // WS trade stream does NOT have a quoteQty field -- it is computed.
    json j = json::parse(R"({"t":1,"p":"2.5","q":"4.0","T":0,"m":false})");
    auto trade = parse_trade_stream(j);

    EXPECT_NEAR(trade.quote_qty, 10.0, 1e-10);
}

// ---------------------------------------------------------------------------
// parse_kline_stream  (<symbol>@kline_<interval>)
// Fields are nested under "k"; single-char keys differ from REST
// ---------------------------------------------------------------------------

TEST(WsParseKlineStreamTest, AllFields) {
    const char* body = R"({
        "e": "kline",
        "E": 123456789,
        "s": "BNBBTC",
        "k": {
            "t": 123400000,
            "T": 123460000,
            "o": "0.0010",
            "c": "0.0020",
            "h": "0.0025",
            "l": "0.0015",
            "v": "1000",
            "q": "1.0000",
            "n": 100,
            "x": false
        }
    })";
    json j = json::parse(body);
    auto kline = parse_kline_stream(j);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(kline.open_time.time_since_epoch()).count(), 123'400'000LL);
    EXPECT_EQ(duration_cast<milliseconds>(kline.close_time.time_since_epoch()).count(), 123'460'000LL);
    EXPECT_NEAR(kline.open, 0.0010, 1e-9);
    EXPECT_NEAR(kline.close, 0.0020, 1e-9);
    EXPECT_NEAR(kline.high, 0.0025, 1e-9);
    EXPECT_NEAR(kline.low, 0.0015, 1e-9);
    EXPECT_NEAR(kline.volume, 1000.0, 1e-6);
    EXPECT_NEAR(kline.quote_volume, 1.0, 1e-9);
    EXPECT_EQ(kline.num_trades, 100U);
}

TEST(WsParseKlineStreamTest, MissingKObjectThrows) {
    // "k" key is accessed with at(), which throws on missing key
    json j = json::parse(R"({"e":"kline"})");
    EXPECT_THROW(parse_kline_stream(j), json::out_of_range);
}

TEST(WsParseKlineStreamTest, MissingSubfieldsUseDefaults) {
    json j = json::parse(R"({"k": {}})");
    auto kline = parse_kline_stream(j);

    EXPECT_DOUBLE_EQ(kline.open, 0.0);
    EXPECT_EQ(kline.num_trades, 0U);
}

// ---------------------------------------------------------------------------
// parse_ticker_stream  (<symbol>@ticker)
// Field differences vs REST: "s" = symbol, "c" = last price, "E" = event time
// ---------------------------------------------------------------------------

TEST(WsParseTickerStreamTest, AllRelevantFields) {
    const char* body = R"({
        "e": "24hrTicker",
        "E": 123456789,
        "s": "BNBBTC",
        "c": "0.0025",
        "p": "0.0003"
    })";
    json j = json::parse(body);
    auto ticker = parse_ticker_stream(j);

    EXPECT_EQ(ticker.symbol, "BNBBTC");
    EXPECT_NEAR(ticker.price, 0.0025, 1e-9);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(ticker.time.time_since_epoch()).count(), 123'456'789LL);
}

TEST(WsParseTickerStreamTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto ticker = parse_ticker_stream(j);

    EXPECT_TRUE(ticker.symbol.empty());
    EXPECT_DOUBLE_EQ(ticker.price, 0.0);
}

TEST(WsParseTickerStreamTest, ZeroPrice) {
    json j = json::parse(R"({"s": "SOLBTC", "c": "0.0", "E": 0})");
    auto ticker = parse_ticker_stream(j);

    EXPECT_EQ(ticker.symbol, "SOLBTC");
    EXPECT_DOUBLE_EQ(ticker.price, 0.0);
}

// ---------------------------------------------------------------------------
// parse_depth_stream  (<symbol>@depth)
// "u" = last_update_id, "b" = bids delta, "a" = asks delta
// ---------------------------------------------------------------------------

TEST(WsParseDepthStreamTest, ValidJson) {
    const char* body = R"({
        "e": "depthUpdate",
        "s": "BNBBTC",
        "U": 157,
        "u": 160,
        "b": [["0.0024","10"],["0.0023","5"]],
        "a": [["0.0026","100"]]
    })";
    json j = json::parse(body);
    auto book = parse_depth_stream(j);

    EXPECT_EQ(book.symbol, "BNBBTC");
    EXPECT_EQ(book.last_update_id, 160U);
    ASSERT_EQ(book.bids.size(), 2U);
    ASSERT_EQ(book.asks.size(), 1U);
    EXPECT_NEAR(book.bids[0].price, 0.0024, 1e-8);
    EXPECT_NEAR(book.bids[0].quantity, 10.0, 1e-6);
    EXPECT_NEAR(book.asks[0].price, 0.0026, 1e-8);
}

TEST(WsParseDepthStreamTest, MissingBidsAndAsksOk) {
    // When "b" and "a" are absent the vectors must remain empty.
    json j = json::parse(R"({"s":"XRPBTC","U":1,"u":1})");
    auto book = parse_depth_stream(j);

    EXPECT_EQ(book.symbol, "XRPBTC");
    EXPECT_TRUE(book.bids.empty());
    EXPECT_TRUE(book.asks.empty());
}

TEST(WsParseDepthStreamTest, EmptyBidsAndAsksArrays) {
    json j = json::parse(R"({"s":"LTCBTC","U":2,"u":2,"b":[],"a":[]})");
    auto book = parse_depth_stream(j);

    EXPECT_TRUE(book.bids.empty());
    EXPECT_TRUE(book.asks.empty());
}

TEST(WsParseDepthStreamTest, LastUpdateIdFromFinalUpdate_u) {
    json j = json::parse(R"({"s":"ADABTC","U":50,"u":99,"b":[],"a":[]})");
    auto book = parse_depth_stream(j);

    // Must use "u" (final update id), not "U" (first update id)
    EXPECT_EQ(book.last_update_id, 99U);
}

// ---------------------------------------------------------------------------
// parse_order_update  (executionReport from user data stream)
// Field differences vs REST: "i" = orderId, "c" = clientOrderId, "S" = side,
//   "o" = type, "f" = timeInForce, "q" = origQty, "p" = price,
//   "z" = executedQty, "Z" = cummulativeQuoteQty, "X" = status,
//   "O" = order time, "T" = transact time
// ---------------------------------------------------------------------------

TEST(WsParseOrderUpdateTest, AllFields) {
    const char* body = R"({
        "e": "executionReport",
        "s": "ETHBTC",
        "i": 4293153,
        "c": "myOrder1",
        "S": "BUY",
        "o": "LIMIT",
        "f": "GTC",
        "q": "1.00000000",
        "p": "0.10264410",
        "z": "0.00000000",
        "Z": "0.00000000",
        "X": "NEW",
        "O": 1499405658658,
        "T": 1499405658657
    })";
    json j = json::parse(body);
    auto order = parse_order_update(j);

    EXPECT_EQ(order.symbol, "ETHBTC");
    EXPECT_EQ(order.order_id, 4'293'153U);
    EXPECT_EQ(order.client_order_id, "myOrder1");
    EXPECT_EQ(order.side, Side::Buy);
    EXPECT_EQ(order.type, OrderType::Limit);
    EXPECT_EQ(order.time_in_force, TimeInForce::GTC);
    EXPECT_NEAR(order.orig_qty, 1.0, 1e-8);
    EXPECT_NEAR(order.price, 0.10264410, 1e-9);
    EXPECT_DOUBLE_EQ(order.executed_qty, 0.0);
    EXPECT_DOUBLE_EQ(order.cummulative_quote_qty, 0.0);
    EXPECT_EQ(order.status, OrderStatus::New);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(order.time.time_since_epoch()).count(), 1'499'405'658'658LL);
    EXPECT_EQ(duration_cast<milliseconds>(order.update_time.time_since_epoch()).count(), 1'499'405'658'657LL);
}

TEST(WsParseOrderUpdateTest, FilledSellOrder) {
    const char* body = R"({
        "s": "BTCUSDT",
        "i": 99,
        "c": "",
        "S": "SELL",
        "o": "MARKET",
        "f": "IOC",
        "q": "0.5",
        "p": "0",
        "z": "0.5",
        "Z": "25000.00",
        "X": "FILLED",
        "O": 0,
        "T": 0
    })";
    json j = json::parse(body);
    auto order = parse_order_update(j);

    EXPECT_EQ(order.side, Side::Sell);
    EXPECT_EQ(order.type, OrderType::Market);
    EXPECT_EQ(order.status, OrderStatus::Filled);
    EXPECT_NEAR(order.executed_qty, 0.5, 1e-8);
    EXPECT_NEAR(order.cummulative_quote_qty, 25000.0, 1e-4);
}

TEST(WsParseOrderUpdateTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto order = parse_order_update(j);

    EXPECT_TRUE(order.symbol.empty());
    EXPECT_EQ(order.order_id, 0U);
    EXPECT_EQ(order.status, OrderStatus::New);
    EXPECT_EQ(order.side, Side::Buy);
}

// ---------------------------------------------------------------------------
// parse_account_update  (outboundAccountPosition from user data stream)
// "u" = update time, "B" array with "a"=asset, "f"=free, "l"=locked
// ---------------------------------------------------------------------------

TEST(WsParseAccountUpdateTest, AllFields) {
    const char* body = R"({
        "e": "outboundAccountPosition",
        "E": 1564034571105,
        "u": 1564034571073,
        "B": [
            {"a": "ETH", "f": "10000.000000", "l": "0.000000"},
            {"a": "USDT", "f": "50.50000000", "l": "100.00000000"}
        ]
    })";
    json j = json::parse(body);
    auto info = parse_account_update(j);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(info.update_time.time_since_epoch()).count(), 1'564'034'571'073LL);

    ASSERT_EQ(info.balances.size(), 2U);
    EXPECT_EQ(info.balances[0].asset, "ETH");
    EXPECT_NEAR(info.balances[0].free, 10000.0, 1e-4);
    EXPECT_DOUBLE_EQ(info.balances[0].locked, 0.0);
    EXPECT_EQ(info.balances[1].asset, "USDT");
    EXPECT_NEAR(info.balances[1].free, 50.5, 1e-6);
    EXPECT_NEAR(info.balances[1].locked, 100.0, 1e-6);
}

TEST(WsParseAccountUpdateTest, MissingBKey) {
    json j = json::parse(R"({"u": 123456})");
    auto info = parse_account_update(j);

    EXPECT_TRUE(info.balances.empty());

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(info.update_time.time_since_epoch()).count(), 123'456LL);
}

TEST(WsParseAccountUpdateTest, EmptyBalancesArray) {
    json j = json::parse(R"({"u": 0, "B": []})");
    auto info = parse_account_update(j);

    EXPECT_TRUE(info.balances.empty());
}

TEST(WsParseAccountUpdateTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto info = parse_account_update(j);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(info.update_time.time_since_epoch()).count(), 0LL);
    EXPECT_TRUE(info.balances.empty());
}

}  // namespace bintrade::test

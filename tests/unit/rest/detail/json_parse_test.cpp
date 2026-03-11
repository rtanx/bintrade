#include "rest/detail/json_parse.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/models/trade.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace bintrade::test {

using nlohmann::json;
using namespace bintrade::rest::detail;

// ---------------------------------------------------------------------------
// check_api_error
// ---------------------------------------------------------------------------

TEST(CheckApiErrorTest, NoThrowOnSuccessStatus) {
    json j = json::parse(R"({"result": "ok"})");
    EXPECT_NO_THROW(check_api_error(200, j));
}

TEST(CheckApiErrorTest, NoThrowOnSuccessStatusWithCodeField) {
    // Non-error status -- code/msg fields may be present but should not throw
    json j = json::parse(R"({"code": 0, "msg": "success"})");
    EXPECT_NO_THROW(check_api_error(200, j));
}

TEST(CheckApiErrorTest, ThrowsApiExceptionOn400WithCodeAndMsg) {
    json j = json::parse(R"({"code": -1100, "msg": "Illegal characters found in parameter"})");
    try {
        check_api_error(400, j);
        FAIL() << "Expected ApiException";
    } catch (const ApiException& e) {
        EXPECT_EQ(e.http_code(), -1'100);
        EXPECT_STREQ(e.what(), "Illegal characters found in parameter");
    }
}

TEST(CheckApiErrorTest, ThrowsApiExceptionOn401) {
    json j = json::parse(R"({"code": -2014, "msg": "API-key format invalid."})");
    EXPECT_THROW(check_api_error(401, j), ApiException);
}

TEST(CheckApiErrorTest, ThrowsApiExceptionOn429) {
    json j = json::parse(R"({"code": -1003, "msg": "Too many requests."})");
    EXPECT_THROW(check_api_error(429, j), ApiException);
}

TEST(CheckApiErrorTest, NoThrowWhenMsgMissing) {
    json j = json::parse(R"({"code": -1100})");
    EXPECT_NO_THROW(check_api_error(400, j));
}

TEST(CheckApiErrorTest, NoThrowWhenCodeMissing) {
    json j = json::parse(R"({"msg": "error"})");
    EXPECT_NO_THROW(check_api_error(400, j));
}

// ---------------------------------------------------------------------------
// parse_response
// ---------------------------------------------------------------------------

TEST(ParseResponseTest, ValidJsonReturnsParsed) {
    auto result = parse_response(200, R"({"serverTime": 1234567890000})");
    EXPECT_EQ(result["serverTime"].get<int64_t>(), 1'234'567'890'000LL);
}

TEST(ParseResponseTest, EmptyBodyWith200ReturnsEmptyJson) {
    auto result = parse_response(200, "");
    EXPECT_TRUE(result.is_null() || result.empty());
}

TEST(ParseResponseTest, EmptyBodyWith400ThrowsApiException) {
    EXPECT_THROW(parse_response(400, ""), ApiException);
}

TEST(ParseResponseTest, ErrorBodyWith400ThrowsApiException) {
    try {
        parse_response(400, R"({"code": -1121, "msg": "Invalid symbol."})");
        FAIL() << "Expected ApiException";
    } catch (const ApiException& e) {
        EXPECT_EQ(e.http_code(), -1'121);
    }
}

// ---------------------------------------------------------------------------
// parse_ticker
// ---------------------------------------------------------------------------

TEST(ParseTickerTest, ValidJson) {
    json j = json::parse(R"({"symbol": "BTCUSDT", "price": "42000.00000000"})");
    auto ticker = parse_ticker(j);

    EXPECT_EQ(ticker.symbol, "BTCUSDT");
    EXPECT_DOUBLE_EQ(ticker.price, 42000.0);
}

TEST(ParseTickerTest, ZeroPrice) {
    json j = json::parse(R"({"symbol": "ETHUSDT", "price": "0.00000000"})");
    auto ticker = parse_ticker(j);
    EXPECT_DOUBLE_EQ(ticker.price, 0.0);
}

TEST(ParseTickerTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto ticker = parse_ticker(j);
    EXPECT_TRUE(ticker.symbol.empty());
    EXPECT_DOUBLE_EQ(ticker.price, 0.0);
}

TEST(ParseTickerTest, HighPrecisionPrice) {
    json j = json::parse(R"({"symbol": "XRPBTC", "price": "0.00002345"})");
    auto ticker = parse_ticker(j);
    EXPECT_NEAR(ticker.price, 0.00002345, 1e-10);
}

// ---------------------------------------------------------------------------
// parse_ticker_24h
// ---------------------------------------------------------------------------

TEST(ParseTicker24hTest, AllFields) {
    const char* body = R"({
        "symbol": "BNBBTC",
        "priceChange": "0.00015000",
        "priceChangePercent": "0.150",
        "openPrice": "0.09900000",
        "highPrice": "0.10200000",
        "lowPrice": "0.09880000",
        "lastPrice": "0.10015000",
        "volume": "12345.60000000",
        "quoteVolume": "1234.56789000",
        "openTime": 1499040000000,
        "closeTime": 1499126400000,
        "count": 10000
    })";
    json j = json::parse(body);
    auto t = parse_ticker_24h(j);

    EXPECT_EQ(t.symbol, "BNBBTC");
    EXPECT_NEAR(t.price_change, 0.00015, 1e-10);
    EXPECT_NEAR(t.price_change_percent, 0.150, 1e-6);
    EXPECT_NEAR(t.open, 0.09900, 1e-8);
    EXPECT_NEAR(t.high, 0.10200, 1e-8);
    EXPECT_NEAR(t.low, 0.09880, 1e-8);
    EXPECT_NEAR(t.close, 0.10015, 1e-8);
    EXPECT_NEAR(t.volume, 12345.6, 1e-4);
    EXPECT_NEAR(t.quote_volume, 1234.56789, 1e-5);
    EXPECT_EQ(t.num_trades, 10'000U);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(t.open_time.time_since_epoch()).count(), 1'499'040'000'000LL);
    EXPECT_EQ(duration_cast<milliseconds>(t.close_time.time_since_epoch()).count(), 1'499'126'400'000LL);
}

TEST(ParseTicker24hTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({"symbol": "TEST"})");
    auto t = parse_ticker_24h(j);
    EXPECT_EQ(t.symbol, "TEST");
    EXPECT_DOUBLE_EQ(t.open, 0.0);
    EXPECT_EQ(t.num_trades, 0U);
}

// ---------------------------------------------------------------------------
// parse_order_book_entry
// ---------------------------------------------------------------------------

TEST(ParseOrderBookEntryTest, ValidArray) {
    json j = json::parse(R"(["0.01720000", "56.00000000"])");
    auto entry = parse_order_book_entry(j);

    EXPECT_NEAR(entry.price, 0.0172, 1e-8);
    EXPECT_NEAR(entry.quantity, 56.0, 1e-8);
}

TEST(ParseOrderBookEntryTest, ZeroQuantityEntry) {
    json j = json::parse(R"(["100.00000000", "0.00000000"])");
    auto entry = parse_order_book_entry(j);

    EXPECT_NEAR(entry.price, 100.0, 1e-6);
    EXPECT_DOUBLE_EQ(entry.quantity, 0.0);
}

// ---------------------------------------------------------------------------
// parse_order_book
// ---------------------------------------------------------------------------

TEST(ParseOrderBookTest, ValidJson) {
    const char* body = R"({
        "lastUpdateId": 1027024,
        "bids": [["4.00000000", "431.00000000"]],
        "asks": [["4.00000200", "12.00000000"]]
    })";
    json j = json::parse(body);
    auto book = parse_order_book(j, "ETHBTC");

    EXPECT_EQ(book.symbol, "ETHBTC");
    EXPECT_EQ(book.last_update_id, 1'027'024U);
    ASSERT_EQ(book.bids.size(), 1U);
    ASSERT_EQ(book.asks.size(), 1U);
    EXPECT_NEAR(book.bids[0].price, 4.0, 1e-6);
    EXPECT_NEAR(book.bids[0].quantity, 431.0, 1e-6);
    EXPECT_NEAR(book.asks[0].price, 4.000002, 1e-8);
    EXPECT_NEAR(book.asks[0].quantity, 12.0, 1e-6);
}

TEST(ParseOrderBookTest, MultipleLevels) {
    const char* body = R"({
        "lastUpdateId": 999,
        "bids": [
            ["10.00", "1.0"],
            ["9.00", "2.0"],
            ["8.00", "3.0"]
        ],
        "asks": [
            ["11.00", "0.5"],
            ["12.00", "0.7"]
        ]
    })";
    json j = json::parse(body);
    auto book = parse_order_book(j, "BNBUSDT");

    EXPECT_EQ(book.bids.size(), 3U);
    EXPECT_EQ(book.asks.size(), 2U);
}

TEST(ParseOrderBookTest, EmptyBidsAndAsks) {
    json j = json::parse(R"({"lastUpdateId": 1, "bids": [], "asks": []})");
    auto book = parse_order_book(j, "LTCBTC");

    EXPECT_TRUE(book.bids.empty());
    EXPECT_TRUE(book.asks.empty());
}

// ---------------------------------------------------------------------------
// parse_trade (REST format: "id", "price", "qty", "time", "isBuyerMaker")
// ---------------------------------------------------------------------------

TEST(ParseTradeTest, ValidJson) {
    const char* body = R"({
        "id": 28457,
        "price": "4.00000100",
        "qty": "12.00000000",
        "quoteQty": "48.00001200",
        "time": 1499865549590,
        "isBuyerMaker": true
    })";
    json j = json::parse(body);
    auto trade = parse_trade(j);

    EXPECT_EQ(trade.id, 28'457U);
    EXPECT_NEAR(trade.price, 4.000001, 1e-8);
    EXPECT_NEAR(trade.qty, 12.0, 1e-8);
    EXPECT_NEAR(trade.quote_qty, 48.00001200, 1e-7);
    EXPECT_TRUE(trade.is_buyer);
    EXPECT_TRUE(trade.is_maker);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(trade.time.time_since_epoch()).count(), 1'499'865'549'590LL);
}

TEST(ParseTradeTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto trade = parse_trade(j);

    EXPECT_EQ(trade.id, 0U);
    EXPECT_DOUBLE_EQ(trade.price, 0.0);
    EXPECT_FALSE(trade.is_buyer);
}

// ---------------------------------------------------------------------------
// parse_kline
// ---------------------------------------------------------------------------

TEST(ParseKlineTest, ValidArrayNineElements) {
    // Binance kline REST array: [openTime, open, high, low, close, vol,
    //                            closeTime, quoteVol, numTrades, ...]
    json j = json::parse(R"([
        1499040000000,
        "0.01634790",
        "0.80000000",
        "0.01575800",
        "0.01577100",
        "148976.11427815",
        1499644799999,
        "2434.19055334",
        308
    ])");
    auto kline = parse_kline(j);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(kline.open_time.time_since_epoch()).count(), 1'499'040'000'000LL);
    EXPECT_NEAR(kline.open, 0.01634790, 1e-9);
    EXPECT_NEAR(kline.high, 0.80000000, 1e-8);
    EXPECT_NEAR(kline.low, 0.01575800, 1e-9);
    EXPECT_NEAR(kline.close, 0.01577100, 1e-9);
    EXPECT_NEAR(kline.volume, 148976.11427815, 1e-5);
    EXPECT_EQ(duration_cast<milliseconds>(kline.close_time.time_since_epoch()).count(), 1'499'644'799'999LL);
    EXPECT_NEAR(kline.quote_volume, 2434.19055334, 1e-5);
    EXPECT_EQ(kline.num_trades, 308U);
}

TEST(ParseKlineTest, ZeroPriceKline) {
    json j = json::parse(R"([0,"0.00000000","0.00000000","0.00000000","0.00000000","0.00000000",0,"0.00000000",0])");
    auto kline = parse_kline(j);

    EXPECT_DOUBLE_EQ(kline.open, 0.0);
    EXPECT_DOUBLE_EQ(kline.high, 0.0);
    EXPECT_EQ(kline.num_trades, 0U);
}

// ---------------------------------------------------------------------------
// parse_order
// ---------------------------------------------------------------------------

TEST(ParseOrderTest, ValidJson) {
    const char* body = R"({
        "symbol": "LTCBTC",
        "orderId": 1,
        "clientOrderId": "myOrder1",
        "price": "0.1",
        "origQty": "1.0",
        "executedQty": "0.0",
        "cummulativeQuoteQty": "0.0",
        "status": "NEW",
        "timeInForce": "GTC",
        "type": "LIMIT",
        "side": "BUY",
        "time": 1499827319559,
        "updateTime": 1499827319559
    })";
    json j = json::parse(body);
    auto order = parse_order(j);

    EXPECT_EQ(order.symbol, "LTCBTC");
    EXPECT_EQ(order.order_id, 1U);
    EXPECT_EQ(order.client_order_id, "myOrder1");
    EXPECT_NEAR(order.price, 0.1, 1e-8);
    EXPECT_NEAR(order.orig_qty, 1.0, 1e-8);
    EXPECT_DOUBLE_EQ(order.executed_qty, 0.0);
    EXPECT_EQ(order.status, OrderStatus::New);
    EXPECT_EQ(order.type, OrderType::Limit);
    EXPECT_EQ(order.side, Side::Buy);
    EXPECT_EQ(order.time_in_force, TimeInForce::GTC);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(order.time.time_since_epoch()).count(), 1'499'827'319'559LL);
}

TEST(ParseOrderTest, FilledMarketSell) {
    const char* body = R"({
        "symbol": "BTCUSDT",
        "orderId": 999,
        "clientOrderId": "",
        "price": "0",
        "origQty": "0.5",
        "executedQty": "0.5",
        "cummulativeQuoteQty": "21000.00",
        "status": "FILLED",
        "timeInForce": "IOC",
        "type": "MARKET",
        "side": "SELL",
        "time": 0,
        "updateTime": 0
    })";
    json j = json::parse(body);
    auto order = parse_order(j);

    EXPECT_EQ(order.status, OrderStatus::Filled);
    EXPECT_EQ(order.type, OrderType::Market);
    EXPECT_EQ(order.side, Side::Sell);
    EXPECT_EQ(order.time_in_force, TimeInForce::IOC);
}

TEST(ParseOrderTest, CanceledStopLossLimit) {
    const char* body = R"({
        "symbol": "BNBBTC",
        "orderId": 7,
        "clientOrderId": "",
        "price": "0.001",
        "origQty": "10.0",
        "executedQty": "0.0",
        "cummulativeQuoteQty": "0.0",
        "status": "CANCELED",
        "timeInForce": "FOK",
        "type": "STOP_LOSS_LIMIT",
        "side": "BUY",
        "time": 0,
        "updateTime": 0
    })";
    json j = json::parse(body);
    auto order = parse_order(j);

    EXPECT_EQ(order.status, OrderStatus::Canceled);
    EXPECT_EQ(order.type, OrderType::StopLossLimit);
    EXPECT_EQ(order.time_in_force, TimeInForce::FOK);
}

TEST(ParseOrderTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto order = parse_order(j);

    EXPECT_TRUE(order.symbol.empty());
    EXPECT_EQ(order.order_id, 0U);
    EXPECT_EQ(order.status, OrderStatus::New);
    EXPECT_EQ(order.type, OrderType::Limit);
    EXPECT_EQ(order.side, Side::Buy);
}

// ---------------------------------------------------------------------------
// parse_balance
// ---------------------------------------------------------------------------

TEST(ParseBalanceTest, ValidJson) {
    json j = json::parse(R"({"asset": "BTC", "free": "1.23456789", "locked": "0.00100000"})");
    auto balance = parse_balance(j);

    EXPECT_EQ(balance.asset, "BTC");
    EXPECT_NEAR(balance.free, 1.23456789, 1e-9);
    EXPECT_NEAR(balance.locked, 0.001, 1e-9);
}

TEST(ParseBalanceTest, ZeroBalance) {
    json j = json::parse(R"({"asset": "ETH", "free": "0.00000000", "locked": "0.00000000"})");
    auto balance = parse_balance(j);

    EXPECT_DOUBLE_EQ(balance.free, 0.0);
    EXPECT_DOUBLE_EQ(balance.locked, 0.0);
}

TEST(ParseBalanceTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto balance = parse_balance(j);

    EXPECT_TRUE(balance.asset.empty());
    EXPECT_DOUBLE_EQ(balance.free, 0.0);
}

// ---------------------------------------------------------------------------
// parse_account_info
// ---------------------------------------------------------------------------

TEST(ParseAccountInfoTest, ValidJson) {
    const char* body = R"({
        "canTrade": true,
        "canWithdraw": false,
        "canDeposit": true,
        "accountType": "SPOT",
        "updateTime": 1564034571073,
        "balances": [
            {"asset": "BTC", "free": "1.00000000", "locked": "0.00000000"},
            {"asset": "ETH", "free": "0.00000000", "locked": "0.00000000"},
            {"asset": "LTC", "free": "5.00000000", "locked": "2.00000000"}
        ]
    })";
    json j = json::parse(body);
    auto info = parse_account_info(j);

    EXPECT_TRUE(info.can_trade);
    EXPECT_FALSE(info.can_withdraw);
    EXPECT_TRUE(info.can_deposit);
    EXPECT_EQ(info.account_type, "SPOT");

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(info.update_time.time_since_epoch()).count(), 1'564'034'571'073LL);

    // Zero-balance ETH entry must be filtered out
    ASSERT_EQ(info.balances.size(), 2U);
    EXPECT_EQ(info.balances[0].asset, "BTC");
    EXPECT_EQ(info.balances[1].asset, "LTC");
}

TEST(ParseAccountInfoTest, EmptyBalancesArray) {
    json j = json::parse(R"({"canTrade": true, "canWithdraw": true, "canDeposit": true, "accountType": "SPOT", "updateTime": 0, "balances": []})");
    auto info = parse_account_info(j);

    EXPECT_TRUE(info.balances.empty());
}

TEST(ParseAccountInfoTest, AllZeroBalancesAreFilteredOut) {
    const char* body = R"({
        "canTrade": false,
        "canWithdraw": false,
        "canDeposit": false,
        "accountType": "SPOT",
        "updateTime": 0,
        "balances": [
            {"asset": "BNB", "free": "0.00000000", "locked": "0.00000000"}
        ]
    })";
    json j = json::parse(body);
    auto info = parse_account_info(j);

    EXPECT_TRUE(info.balances.empty());
}

// ---------------------------------------------------------------------------
// parse_account_trade
// ---------------------------------------------------------------------------

TEST(ParseAccountTradeTest, ValidJson) {
    const char* body = R"({
        "symbol": "BNBBTC",
        "id": 28457,
        "orderId": 100234,
        "price": "4.00000100",
        "qty": "12.00000000",
        "quoteQty": "48.00001200",
        "commission": "10.10000000",
        "commissionAsset": "BNB",
        "time": 1499865549590,
        "isBuyer": true,
        "isMaker": false
    })";
    json j = json::parse(body);
    auto trade = parse_account_trade(j);

    EXPECT_EQ(trade.symbol, "BNBBTC");
    EXPECT_EQ(trade.id, 28'457U);
    EXPECT_EQ(trade.order_id, 100'234U);
    EXPECT_NEAR(trade.price, 4.000001, 1e-7);
    EXPECT_NEAR(trade.qty, 12.0, 1e-6);
    EXPECT_NEAR(trade.quote_qty, 48.00001200, 1e-7);
    EXPECT_NEAR(trade.commission, 10.1, 1e-7);
    EXPECT_EQ(trade.commission_asset, "BNB");
    EXPECT_TRUE(trade.is_buyer);
    EXPECT_FALSE(trade.is_maker);

    using namespace std::chrono;
    EXPECT_EQ(duration_cast<milliseconds>(trade.time.time_since_epoch()).count(), 1'499'865'549'590LL);
}

TEST(ParseAccountTradeTest, MissingFieldsUseDefaults) {
    json j = json::parse(R"({})");
    auto trade = parse_account_trade(j);

    EXPECT_TRUE(trade.symbol.empty());
    EXPECT_EQ(trade.id, 0U);
    EXPECT_DOUBLE_EQ(trade.commission, 0.0);
    EXPECT_FALSE(trade.is_buyer);
    EXPECT_FALSE(trade.is_maker);
}

}  // namespace bintrade::test

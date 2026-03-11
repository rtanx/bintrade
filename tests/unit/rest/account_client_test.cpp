// Tests for bintrade::rest::AccountClient -- account info and trade history.

#include "binance_responses.hpp"
#include "mock_http_transport.hpp"
#include "rest/detail/http_transport.hpp"

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/rest/account_client.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace testing;
using namespace bintrade;
using namespace bintrade::rest;
using bintrade::rest::detail::HttpResponse;
using bintrade::rest::detail::HttpTransport;
using bintrade::test::MockHttpTransport;

namespace {

class TestableAccountClient : public AccountClient {
public:
    explicit TestableAccountClient(std::unique_ptr<HttpTransport> t) : AccountClient(std::move(t)) {}
    TestableAccountClient(std::unique_ptr<HttpTransport> t, Credentials c) : AccountClient(std::move(t), std::move(c)) {}
};

// Re-use the same NiceMock+credentials helper pattern from trading_client_test.
struct AuthFixture {
    NiceMock<MockHttpTransport>* raw{};
    TestableAccountClient client;

    AuthFixture()
        : client([this]() -> TestableAccountClient {
              auto mock = std::make_unique<NiceMock<MockHttpTransport>>();
              raw = mock.get();
              return {std::move(mock), Credentials("test-api-key", "test-secret-32chars-padded000")};
          }()) {}
};

// ---------------------------------------------------------------------------
// get_account_info
// ---------------------------------------------------------------------------
TEST(AccountClientTest, GetAccountInfoCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/account", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::account_info_response)}));

    auto info = fx.client.get_account_info();
    EXPECT_TRUE(info.can_trade);
    EXPECT_TRUE(info.can_withdraw);
    EXPECT_TRUE(info.can_deposit);
    EXPECT_EQ(info.account_type, "SPOT");
}

TEST(AccountClientTest, GetAccountInfoParsesNonZeroBalancesOnly) {
    // Fixture has BTC (non-zero), USDT (non-zero), ETH (all zero -- excluded).
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/account", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::account_info_response)}));

    auto info = fx.client.get_account_info();
    ASSERT_EQ(info.balances.size(), 2U);

    // Parser skips balances where free == 0 && locked == 0.
    EXPECT_EQ(info.balances[0].asset, "BTC");
    EXPECT_DOUBLE_EQ(info.balances[0].free, 1.5);
    EXPECT_DOUBLE_EQ(info.balances[0].locked, 0.5);

    EXPECT_EQ(info.balances[1].asset, "USDT");
    EXPECT_DOUBLE_EQ(info.balances[1].free, 50000.0);
    EXPECT_DOUBLE_EQ(info.balances[1].locked, 10000.0);
}

TEST(AccountClientTest, GetAccountInfoThrowsApiExceptionOn401) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/account", _)).WillOnce(Return(HttpResponse{401, R"({"code":-2014,"msg":"API-key format invalid."})"}));

    EXPECT_THROW((void)fx.client.get_account_info(), ApiException);
}

// ---------------------------------------------------------------------------
// get_account_trades
// ---------------------------------------------------------------------------
TEST(AccountClientTest, GetAccountTradesCallsCorrectEndpoint) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/myTrades", _)).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::account_trades_response)}));

    auto trades = fx.client.get_account_trades("BTCUSDT");
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].symbol, "BTCUSDT");
    EXPECT_EQ(trades[0].id, 200'000'001ULL);
    EXPECT_EQ(trades[0].order_id, 123'456'789ULL);
    EXPECT_DOUBLE_EQ(trades[0].price, 42000.0);
    EXPECT_DOUBLE_EQ(trades[0].qty, 1.0);
    EXPECT_DOUBLE_EQ(trades[0].commission, 0.001);
    EXPECT_EQ(trades[0].commission_asset, "BTC");
    EXPECT_TRUE(trades[0].is_buyer);
    EXPECT_FALSE(trades[0].is_maker);
}

TEST(AccountClientTest, GetAccountTradesPassesSymbolAndLimitParams) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/myTrades", AllOf(Contains(Key("symbol")), Contains(Key("limit"))))).WillOnce(Return(HttpResponse{200, "[]"}));

    EXPECT_TRUE(fx.client.get_account_trades("BTCUSDT", 100).empty());
}

TEST(AccountClientTest, GetAccountTradesReturnsEmptyVectorForEmptyArray) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/myTrades", _)).WillOnce(Return(HttpResponse{200, "[]"}));

    EXPECT_TRUE(fx.client.get_account_trades("BTCUSDT").empty());
}

TEST(AccountClientTest, GetAccountTradesThrowsApiExceptionOnError) {
    AuthFixture fx;
    EXPECT_CALL(*fx.raw, get("/api/v3/myTrades", _)).WillOnce(Return(HttpResponse{400, std::string(test::fixtures::api_error_response)}));

    EXPECT_THROW((void)fx.client.get_account_trades("BADXYZ"), ApiException);
}

}  // namespace

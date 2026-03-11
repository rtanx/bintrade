// Tests for bintrade::rest::Client -- ping(), server_time(), and error propagation.
//
// Uses MockHttpTransport injected via the protected transport-injection
// constructor to test client logic without any network I/O.

#include "binance_responses.hpp"
#include "mock_http_transport.hpp"
#include "rest/detail/http_transport.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/rest/client.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

using namespace testing;
using namespace bintrade;
using namespace bintrade::rest;
using bintrade::rest::detail::HttpResponse;
using bintrade::rest::detail::HttpTransport;
using bintrade::test::MockHttpTransport;

namespace {

// Exposes the protected transport-injection constructor publicly.
class TestableClient : public Client {
public:
    explicit TestableClient(std::unique_ptr<HttpTransport> t) : Client(std::move(t)) {}
    TestableClient(std::unique_ptr<HttpTransport> t, Credentials c) : Client(std::move(t), std::move(c)) {}
};

// ---------------------------------------------------------------------------
// ping()
// ---------------------------------------------------------------------------
TEST(ClientPingTest, ReturnsTrueWhenStatusIs200) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ping", IsEmpty())).WillOnce(Return(HttpResponse{200, "{}"}));
    TestableClient client(std::move(mock));
    EXPECT_TRUE(client.ping());
}

TEST(ClientPingTest, ReturnsFalseWhenStatusIsNot200) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ping", _)).WillOnce(Return(HttpResponse{503, ""}));
    TestableClient client(std::move(mock));
    EXPECT_FALSE(client.ping());
}

// ---------------------------------------------------------------------------
// server_time()
// ---------------------------------------------------------------------------
TEST(ClientServerTimeTest, ParsesServerTimeFromResponse) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/time", IsEmpty())).WillOnce(Return(HttpResponse{200, std::string(test::fixtures::server_time_response)}));
    TestableClient client(std::move(mock));

    auto ts = client.server_time();
    using namespace std::chrono;
    auto ms = duration_cast<milliseconds>(ts.time_since_epoch());
    EXPECT_EQ(ms.count(), 1'704'067'200'000LL);
}

TEST(ClientServerTimeTest, ThrowsApiExceptionOnErrorResponse) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/time", _)).WillOnce(Return(HttpResponse{400, std::string(test::fixtures::api_error_response)}));
    TestableClient client(std::move(mock));

    EXPECT_THROW((void)client.server_time(), ApiException);
}

// ---------------------------------------------------------------------------
// Error propagation through public_get (tested via a derived client method)
// ---------------------------------------------------------------------------

// Minimal concrete subclass that exposes public_get for error-propagation
// testing without pulling in a full REST subclass.
class ClientWithPublicGet : public Client {
public:
    explicit ClientWithPublicGet(std::unique_ptr<HttpTransport> t) : Client(std::move(t)) {}

    std::string exposed_public_get(const std::string& path, const Params& params = {}) { return public_get(path, params); }
};

TEST(ClientErrorTest, PublicGetThrowsApiExceptionOn400WithCode) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get(_, _)).WillOnce(Return(HttpResponse{.status_code = 400, .body = std::string(test::fixtures::api_error_response)}));
    ClientWithPublicGet client(std::move(mock));

    EXPECT_THROW(client.exposed_public_get("/api/v3/anything"), ApiException);
}

TEST(ClientErrorTest, PublicGetThrowsApiExceptionOn429RateLimit) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get(_, _)).WillOnce(Return(HttpResponse{.status_code = 429, .body = std::string(test::fixtures::rate_limit_error_response)}));
    ClientWithPublicGet client(std::move(mock));

    EXPECT_THROW(client.exposed_public_get("/api/v3/anything"), ApiException);
}

TEST(ClientErrorTest, PublicGetThrowsApiExceptionOn401) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get(_, _)).WillOnce(Return(HttpResponse{.status_code = 401, .body = R"({"code":-2014,"msg":"API-key format invalid."})"}));
    ClientWithPublicGet client(std::move(mock));

    EXPECT_THROW(client.exposed_public_get("/api/v3/anything"), ApiException);
}

TEST(ClientErrorTest, PublicGetReturnsBodyOn200) {
    auto mock = std::make_unique<MockHttpTransport>();
    const std::string body = R"({"result":"ok"})";
    EXPECT_CALL(*mock, get(_, _)).WillOnce(Return(HttpResponse{.status_code = 200, .body = body}));
    ClientWithPublicGet client(std::move(mock));

    EXPECT_EQ(client.exposed_public_get("/api/v3/anything"), body);
}

// ---------------------------------------------------------------------------
// Credentials injection
// ---------------------------------------------------------------------------
TEST(ClientCredentialsTest, SetApiKeyCalledDuringConstructionWithCredentials) {
    auto mock = std::make_unique<StrictMock<MockHttpTransport>>();
    EXPECT_CALL(*mock, set_api_key("test-api-key")).Times(1);
    // No other calls expected -- transport is never used in this test.
    Credentials creds("test-api-key", "test-secret");
    TestableClient client(std::move(mock), std::move(creds));
}

// ---------------------------------------------------------------------------
// Rate-limit header tracking
// ---------------------------------------------------------------------------
TEST(ClientRateLimitTest, UsedWeightUpdatedFromPingResponse) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ping", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = "{}", .headers = {{"x-mbx-used-weight-1m", "42"}}}));
    TestableClient client(std::move(mock));
    EXPECT_TRUE(client.ping());
    EXPECT_EQ(client.used_weight_1m(), 42);
}

TEST(ClientRateLimitTest, OrderCountsUpdatedFromSignedResponse) {
    auto mock_raw = std::make_unique<MockHttpTransport>();
    auto* raw = mock_raw.get();
    EXPECT_CALL(*raw, set_api_key(_));
    EXPECT_CALL(*raw, get("/api/v3/time", _))
        .WillOnce(Return(HttpResponse{.status_code = 200,
                                      .body = std::string(test::fixtures::server_time_response),
                                      .headers = {{"x-mbx-order-count-10s", "5"}, {"x-mbx-order-count-1d", "1000"}}}));
    Credentials creds("test-key", "test-secret");
    TestableClient client(std::move(mock_raw), std::move(creds));
    [[maybe_unused]] auto ts = client.server_time();
    EXPECT_EQ(client.order_count_10s(), 5);
    EXPECT_EQ(client.order_count_1d(), 1'000);
}

TEST(ClientRateLimitTest, RateLimitCountersAreZeroBeforeAnyRequest) {
    auto mock = std::make_unique<MockHttpTransport>();
    TestableClient client(std::move(mock));
    EXPECT_EQ(client.used_weight_1m(), 0);
    EXPECT_EQ(client.order_count_10s(), 0);
    EXPECT_EQ(client.order_count_1d(), 0);
}

TEST(ClientRateLimitTest, MalformedRateLimitHeaderIsIgnored) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ping", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = "{}", .headers = {{"x-mbx-used-weight-1m", "not-a-number"}}}));
    TestableClient client(std::move(mock));
    EXPECT_TRUE(client.ping());
    // stoi failure → -1 sentinel; weight should not remain 0
    EXPECT_EQ(client.used_weight_1m(), -1);
}

TEST(ClientRateLimitTest, MissingRateLimitHeadersLeaveCountersUnchanged) {
    auto mock = std::make_unique<MockHttpTransport>();
    EXPECT_CALL(*mock, get("/api/v3/ping", _))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = "{}"}))
        .WillOnce(Return(HttpResponse{.status_code = 200, .body = "{}", .headers = {{"x-mbx-used-weight-1m", "10"}}}));
    TestableClient client(std::move(mock));
    EXPECT_TRUE(client.ping());  // no rate-limit header
    EXPECT_EQ(client.used_weight_1m(), 0);
    EXPECT_TRUE(client.ping());  // now has header
    EXPECT_EQ(client.used_weight_1m(), 10);
}

}  // namespace

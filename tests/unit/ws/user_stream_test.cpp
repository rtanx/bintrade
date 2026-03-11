// Unit tests for bintrade::ws::UserStream
//
// Tests focus on the logic that is exercisable without a live network
// connection:
//   - Authentication guard in start()
//   - keep_alive() no-op contract before start
//   - stop() safety before start
//   - dispatch_message routing to registered callbacks
//   - dispatch_message: malformed JSON is silently discarded

#include "binance_responses.hpp"
#include "mock_http_transport.hpp"

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/ws/user_stream.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <string_view>

namespace {

using namespace bintrade;
using namespace bintrade::ws;

// ---------------------------------------------------------------------------
// TestableUserStream
// Exposes the protected dispatch_message method for direct testing.
// ---------------------------------------------------------------------------
class TestableUserStream : public UserStream {
public:
    using UserStream::UserStream;

    void test_dispatch(std::string_view msg) { dispatch_message(msg); }
};

// ---------------------------------------------------------------------------
// Authentication guard
// ---------------------------------------------------------------------------
TEST(UserStreamTest, StartThrowsAuthenticationExceptionWithEmptyCredentials) {
    Credentials empty_creds;
    UserStream stream(std::move(empty_creds));
    EXPECT_THROW(stream.start(), AuthenticationException);
}

// ---------------------------------------------------------------------------
// keep_alive() before start
// ---------------------------------------------------------------------------
TEST(UserStreamTest, KeepAliveIsNoOpBeforeStart) {
    // listen_key_ is empty at construction; keep_alive() must return without
    // attempting any HTTP call.
    Credentials creds("test-key", "test-secret");
    UserStream stream(std::move(creds));
    EXPECT_NO_THROW(stream.keep_alive());
}

// ---------------------------------------------------------------------------
// stop() before start
// ---------------------------------------------------------------------------
TEST(UserStreamTest, StopIsNoOpBeforeStart) {
    // Neither HTTP call nor WebSocket disconnect should be triggered.
    Credentials creds("test-key", "test-secret");
    UserStream stream(std::move(creds));
    EXPECT_NO_THROW(stream.stop());
}

// ---------------------------------------------------------------------------
// dispatch_message: order update routing
// ---------------------------------------------------------------------------
TEST(UserStreamTest, DispatchMessageRoutesExecutionReportToOrderCallback) {
    Credentials empty_creds;
    TestableUserStream stream(std::move(empty_creds));

    std::atomic<int> call_count{0};
    std::string received_symbol;

    stream.set_order_update_callback([&](const models::Order& order) {
        ++call_count;
        received_symbol = order.symbol;
    });

    stream.test_dispatch(test::fixtures::ws_order_update);

    EXPECT_EQ(call_count.load(), 1);
    EXPECT_EQ(received_symbol, "BTCUSDT");
}

// ---------------------------------------------------------------------------
// dispatch_message: account update routing
// ---------------------------------------------------------------------------
TEST(UserStreamTest, DispatchMessageRoutesAccountPositionToAccountCallback) {
    Credentials empty_creds;
    TestableUserStream stream(std::move(empty_creds));

    std::atomic<int> call_count{0};
    std::size_t balance_count = 0;

    stream.set_account_update_callback([&](const models::AccountInfo& info) {
        ++call_count;
        balance_count = info.balances.size();
    });

    stream.test_dispatch(test::fixtures::ws_account_update);

    EXPECT_EQ(call_count.load(), 1);
    EXPECT_EQ(balance_count, 2U);
}

// ---------------------------------------------------------------------------
// dispatch_message: unknown event type
// ---------------------------------------------------------------------------
TEST(UserStreamTest, DispatchMessageIgnoresUnknownEventType) {
    Credentials empty_creds;
    TestableUserStream stream(std::move(empty_creds));

    std::atomic<int> order_calls{0};
    std::atomic<int> account_calls{0};

    stream.set_order_update_callback([&](const models::Order&) { ++order_calls; });
    stream.set_account_update_callback([&](const models::AccountInfo&) { ++account_calls; });

    constexpr std::string_view unknown = R"({"e":"unknownEvent","data":{}})";
    EXPECT_NO_THROW(stream.test_dispatch(unknown));

    EXPECT_EQ(order_calls.load(), 0);
    EXPECT_EQ(account_calls.load(), 0);
}

// ---------------------------------------------------------------------------
// dispatch_message: malformed JSON
// ---------------------------------------------------------------------------
TEST(UserStreamTest, DispatchMessageSilentlyDiscardsInvalidJson) {
    Credentials empty_creds;
    TestableUserStream stream(std::move(empty_creds));

    std::atomic<int> order_calls{0};
    stream.set_order_update_callback([&](const models::Order&) { ++order_calls; });

    // Should not throw; the exception is caught internally.
    EXPECT_NO_THROW(stream.test_dispatch("{{not valid json}}"));
    EXPECT_EQ(order_calls.load(), 0);
}

// ---------------------------------------------------------------------------
// dispatch_message: no callback registered
// ---------------------------------------------------------------------------
TEST(UserStreamTest, DispatchMessageIsNoOpWhenNoCallbackRegistered) {
    Credentials empty_creds;
    TestableUserStream stream(std::move(empty_creds));

    // No callbacks registered; must not crash.
    EXPECT_NO_THROW(stream.test_dispatch(test::fixtures::ws_order_update));
    EXPECT_NO_THROW(stream.test_dispatch(test::fixtures::ws_account_update));
}

// ---------------------------------------------------------------------------
// Listen-key lifecycle tests with injected MockHttpTransport
//
// TestableUserStreamWithMock overrides do_connect() to avoid real network I/O
// and records which path was passed so we can verify the listen-key URL.
// ---------------------------------------------------------------------------
class TestableUserStreamWithMock : public UserStream {
public:
    // Explicit public constructor that calls the protected injection constructor.
    TestableUserStreamWithMock(Credentials credentials, std::unique_ptr<rest::detail::HttpTransport> http,
                               WebSocketConfig ws_config = WebSocketConfig{})
        : UserStream(std::move(credentials), std::move(http), std::move(ws_config)) {}

    std::string last_connected_path;

    void test_dispatch(std::string_view msg) { dispatch_message(msg); }

protected:
    void do_connect(std::string_view stream_name) override {
        last_connected_path = std::string(stream_name);
        // No real network I/O.
    }
};

TEST(UserStreamTest, StartCallsCreateListenKeyAndConnectsToListenKeyPath) {
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    Credentials creds("key", "secret");

    auto mock = std::make_unique<NiceMock<test::MockHttpTransport>>();
    auto* raw = mock.get();

    // POST /api/v3/userDataStream returns a listen key.
    constexpr std::string_view create_resp = R"({"listenKey":"test-listen-key-abc"})";
    EXPECT_CALL(*raw, post("/api/v3/userDataStream", _, _)).WillOnce(Return(rest::detail::HttpResponse{200, std::string(create_resp), {}}));

    TestableUserStreamWithMock stream(std::move(creds), std::move(mock));
    stream.start();

    EXPECT_EQ(stream.last_connected_path, "/ws/test-listen-key-abc");

    stream.stop();  // join keep_alive_thread_ before destruction
}

TEST(UserStreamTest, KeepAliveCallsRenewListenKeyEndpoint) {
    using ::testing::_;
    using ::testing::HasSubstr;
    using ::testing::NiceMock;
    using ::testing::Return;

    Credentials creds("key", "secret");

    auto mock = std::make_unique<NiceMock<test::MockHttpTransport>>();
    auto* raw = mock.get();

    constexpr std::string_view create_resp = R"({"listenKey":"test-listen-key-abc"})";
    EXPECT_CALL(*raw, post("/api/v3/userDataStream", _, _)).WillOnce(Return(rest::detail::HttpResponse{200, std::string(create_resp), {}}));

    // The keep-alive renewel call contains the listen key in the path.
    EXPECT_CALL(*raw, post(HasSubstr("test-listen-key-abc"), _, _)).WillOnce(Return(rest::detail::HttpResponse{200, "", {}}));

    TestableUserStreamWithMock stream(std::move(creds), std::move(mock));
    stream.start();
    stream.keep_alive();

    stream.stop();  // join keep_alive_thread_ before destruction
}

TEST(UserStreamTest, StopCallsDeleteListenKeyEndpoint) {
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;

    Credentials creds("key", "secret");

    auto mock = std::make_unique<NiceMock<test::MockHttpTransport>>();
    auto* raw = mock.get();

    constexpr std::string_view create_resp = R"({"listenKey":"test-listen-key-abc"})";
    EXPECT_CALL(*raw, post("/api/v3/userDataStream", _, _)).WillOnce(Return(rest::detail::HttpResponse{200, std::string(create_resp), {}}));

    // DELETE /api/v3/userDataStream with listenKey param.
    EXPECT_CALL(*raw, del("/api/v3/userDataStream", _)).WillOnce(Return(rest::detail::HttpResponse{200, "", {}}));

    TestableUserStreamWithMock stream(std::move(creds), std::move(mock));
    stream.start();
    stream.stop();
}

}  // namespace

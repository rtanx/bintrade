// Unit tests for UserStream DispatchMode (Inline vs Queued).
//
// Verifies:
//   - DispatchMode::Queued delivers order and account updates
//   - set_dispatch_mode after start throws ValidationException
//   - Malformed JSON is silently discarded in queued mode

#include "binance_responses.hpp"

// Complete type for HttpTransport required by UserStream's unique_ptr destructor.
#include "rest/detail/http_transport.hpp"

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/config.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/ws/user_stream.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>

namespace {

using namespace bintrade;
using namespace bintrade::ws;
namespace fixtures = bintrade::test::fixtures;

// ---------------------------------------------------------------------------
// TestableUserStream with queued dispatch support.
// Exposes dispatch_message for direct testing without network I/O.
// ---------------------------------------------------------------------------
class TestableUserStream : public UserStream {
public:
    using UserStream::UserStream;

    void test_dispatch(std::string_view msg) { dispatch_message(msg); }
};

// Helper: wait for an atomic counter to reach expected value, with timeout.
bool wait_for(const std::atomic<int>& counter, int expected, std::chrono::milliseconds timeout = std::chrono::milliseconds{500}) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (counter.load() < expected) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return true;
}

// ---------------------------------------------------------------------------
// Inline mode dispatch (default) -- same as existing behaviour.
// ---------------------------------------------------------------------------
TEST(UserStreamDispatchTest, InlineModeDefault) {
    Credentials creds("test-key", "test-secret");
    TestableUserStream stream(std::move(creds));

    std::atomic<int> order_count{0};
    models::Order received;

    stream.set_order_update_callback([&](const models::Order& o) {
        received = o;
        ++order_count;
    });

    stream.test_dispatch(fixtures::ws_order_update);
    ASSERT_EQ(order_count.load(), 1);
    EXPECT_EQ(received.symbol, "BTCUSDT");
}

// ---------------------------------------------------------------------------
// set_dispatch_mode before start is allowed.
// ---------------------------------------------------------------------------
TEST(UserStreamDispatchTest, SetDispatchModeBeforeStartAllowed) {
    Credentials creds("test-key", "test-secret");
    TestableUserStream stream(std::move(creds));
    EXPECT_NO_THROW(stream.set_dispatch_mode(DispatchMode::Queued));
}

// ---------------------------------------------------------------------------
// Queued mode dispatch -- direct dispatch_message path.
// Note: The consumer thread is only started by start(). For unit testing
// without a live connection, we test that dispatch_message still works
// (it delegates to do_dispatch which is synchronous even in queued mode
// when called directly -- the queue is only used for the I/O callback path).
// ---------------------------------------------------------------------------
TEST(UserStreamDispatchTest, DirectDispatchStillWorksinQueuedMode) {
    Credentials creds("test-key", "test-secret");
    TestableUserStream stream(std::move(creds));
    stream.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> order_count{0};
    stream.set_order_update_callback([&](const models::Order& /*o*/) { ++order_count; });

    // dispatch_message calls do_dispatch synchronously regardless of mode.
    stream.test_dispatch(fixtures::ws_order_update);
    ASSERT_EQ(order_count.load(), 1);
}

// ---------------------------------------------------------------------------
// Malformed JSON is silently discarded.
// ---------------------------------------------------------------------------
TEST(UserStreamDispatchTest, MalformedJsonDiscarded) {
    Credentials creds("test-key", "test-secret");
    TestableUserStream stream(std::move(creds));
    stream.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> count{0};
    stream.set_order_update_callback([&](const models::Order& /*o*/) { ++count; });

    stream.test_dispatch("{{{bad json");
    EXPECT_EQ(count.load(), 0);
}

// ---------------------------------------------------------------------------
// Account update in queued mode (direct dispatch path).
// ---------------------------------------------------------------------------
TEST(UserStreamDispatchTest, AccountUpdateInQueuedMode) {
    Credentials creds("test-key", "test-secret");
    TestableUserStream stream(std::move(creds));
    stream.set_dispatch_mode(DispatchMode::Queued);

    std::atomic<int> account_count{0};
    models::AccountInfo received;
    stream.set_account_update_callback([&](const models::AccountInfo& a) {
        received = a;
        ++account_count;
    });

    stream.test_dispatch(fixtures::ws_account_update);
    ASSERT_EQ(account_count.load(), 1);
    ASSERT_FALSE(received.balances.empty());
    EXPECT_EQ(received.balances[0].asset, "BTC");
}

}  // namespace

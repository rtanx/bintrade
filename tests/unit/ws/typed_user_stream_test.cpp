// Unit tests for bintrade::ws::TypedUserStream<Handler>
//
// Verifies:
//   - Order update events dispatched to handler.on_order_update()
//   - Account update events dispatched to handler.on_account_update()
//   - Malformed JSON is silently discarded
//   - Partial handlers (only one on_* method defined)

#include "binance_responses.hpp"

// Complete type for HttpTransport required by UserStream's unique_ptr destructor.
#include "rest/detail/http_transport.hpp"

#include <bintrade/models/account.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/ws/typed_user_stream.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <string_view>

namespace {

using namespace bintrade;
using namespace bintrade::ws;
namespace fixtures = bintrade::test::fixtures;

// ---------------------------------------------------------------------------
// Full handler with both on_* methods.
// ---------------------------------------------------------------------------
struct FullUserHandler {
    std::atomic<int> order_count{0};
    std::atomic<int> account_count{0};

    models::Order last_order;
    models::AccountInfo last_account;

    void on_order_update(const models::Order& o) {
        last_order = o;
        ++order_count;
    }
    void on_account_update(const models::AccountInfo& a) {
        last_account = a;
        ++account_count;
    }
};

// ---------------------------------------------------------------------------
// Partial handler: only on_order_update.
// ---------------------------------------------------------------------------
struct OrderOnlyHandler {
    std::atomic<int> order_count{0};
    models::Order last_order;

    void on_order_update(const models::Order& o) {
        last_order = o;
        ++order_count;
    }
};

// ---------------------------------------------------------------------------
// TestableTypedUserStream -- suppresses network I/O.
// Uses a simple dummy Credentials and exposes dispatch for direct testing.
// ---------------------------------------------------------------------------
template <typename Handler>
class TestableTypedUserStream : public TypedUserStream<Handler> {
public:
    TestableTypedUserStream(Handler& handler, Credentials credentials) : TypedUserStream<Handler>(handler, std::move(credentials)) {}

    void test_dispatch(std::string_view msg) { this->dispatch_message(msg); }
};

// ---------------------------------------------------------------------------
// Order update dispatch
// ---------------------------------------------------------------------------
TEST(TypedUserStreamTest, DispatchesOrderUpdate) {
    FullUserHandler h;
    Credentials creds("test-key", "test-secret");
    TestableTypedUserStream<FullUserHandler> stream(h, std::move(creds));

    stream.test_dispatch(fixtures::ws_order_update);

    ASSERT_EQ(h.order_count.load(), 1);
    EXPECT_EQ(h.last_order.symbol, "BTCUSDT");
    EXPECT_EQ(h.last_order.order_id, 123'456'789UL);
}

// ---------------------------------------------------------------------------
// Account update dispatch
// ---------------------------------------------------------------------------
TEST(TypedUserStreamTest, DispatchesAccountUpdate) {
    FullUserHandler h;
    Credentials creds("test-key", "test-secret");
    TestableTypedUserStream<FullUserHandler> stream(h, std::move(creds));

    stream.test_dispatch(fixtures::ws_account_update);

    ASSERT_EQ(h.account_count.load(), 1);
    ASSERT_FALSE(h.last_account.balances.empty());
    EXPECT_EQ(h.last_account.balances[0].asset, "BTC");
}

// ---------------------------------------------------------------------------
// Malformed JSON
// ---------------------------------------------------------------------------
TEST(TypedUserStreamTest, MalformedJsonSilentlyDiscarded) {
    FullUserHandler h;
    Credentials creds("test-key", "test-secret");
    TestableTypedUserStream<FullUserHandler> stream(h, std::move(creds));

    stream.test_dispatch("{{{bad json");

    EXPECT_EQ(h.order_count.load(), 0);
    EXPECT_EQ(h.account_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Non-matching event type
// ---------------------------------------------------------------------------
TEST(TypedUserStreamTest, IgnoresNonMatchingEventType) {
    FullUserHandler h;
    Credentials creds("test-key", "test-secret");
    TestableTypedUserStream<FullUserHandler> stream(h, std::move(creds));

    // Inject a trade stream message -- not an executionReport or accountPosition.
    stream.test_dispatch(fixtures::ws_trade_stream);

    EXPECT_EQ(h.order_count.load(), 0);
    EXPECT_EQ(h.account_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Partial handler: only on_order_update
// ---------------------------------------------------------------------------
TEST(TypedUserStreamTest, PartialHandlerOrderOnly) {
    OrderOnlyHandler h;
    Credentials creds("test-key", "test-secret");
    TestableTypedUserStream<OrderOnlyHandler> stream(h, std::move(creds));

    stream.test_dispatch(fixtures::ws_order_update);
    ASSERT_EQ(h.order_count.load(), 1);

    // Account update should be silently discarded -- no on_account_update method.
    stream.test_dispatch(fixtures::ws_account_update);
    EXPECT_EQ(h.order_count.load(), 1);  // Unchanged.
}

}  // namespace

#pragma once

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/config.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/ws/client.hpp>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// Forward-declare the internal HTTP transport in its actual namespace so the
// protected test-injection constructor can accept it without pulling
// Boost/Beast into public headers.
namespace bintrade::rest::detail {
class HttpTransport;
}  // namespace bintrade::rest::detail

namespace bintrade::ws {

using AccountUpdateCallback = std::function<void(const models::AccountInfo& account)>;
using OrderUpdateCallback = std::function<void(const models::Order& order)>;

class UserStream : public Client {
public:
    // rest_config is used for the listen-key lifecycle REST calls
    // (POST/PUT/DELETE /api/v3/userDataStream).
    explicit UserStream(Credentials credentials, RestConfig rest_config = RestConfig{}, WebSocketConfig ws_config = WebSocketConfig{});
    ~UserStream() override;

    UserStream(const UserStream&) = delete;
    UserStream& operator=(const UserStream&) = delete;
    UserStream(UserStream&&) = delete;
    UserStream& operator=(UserStream&&) = delete;

    // Set the dispatch mode. Must be called before start().
    // Default is DispatchMode::Inline (current behaviour).
    void set_dispatch_mode(DispatchMode mode);

    // Start the user data stream:
    //   1. POST /api/v3/userDataStream -> obtain listenKey
    //   2. Connect WebSocket to /ws/<listenKey>
    //   3. Start keep-alive timer (renews listen key every 30 minutes)
    void start();

    // Renew the listen key (PUT /api/v3/userDataStream).
    // Called automatically by the internal timer; exposed for manual control.
    void keep_alive();

    // Stop the user data stream:
    //   1. DELETE /api/v3/userDataStream (invalidates listen key)
    //   2. Disconnect WebSocket
    void stop();

    void set_account_update_callback(AccountUpdateCallback callback);
    void set_order_update_callback(OrderUpdateCallback callback);

private:
    Credentials credentials_;
    RestConfig rest_config_;
    std::string listen_key_;
    AccountUpdateCallback account_callback_;
    OrderUpdateCallback order_callback_;

    DispatchMode dispatch_mode_ = DispatchMode::Inline;
    bool started_ = false;  // Guards set_dispatch_mode after start.

    struct QueueState;
    std::unique_ptr<QueueState> queue_state_;

    void start_consumer();
    void stop_consumer();

    // Injected HTTP transport (non-null only when constructed via the
    // test-injection constructor; nullptr in production).
    std::unique_ptr<::bintrade::rest::detail::HttpTransport> rest_http_;

    // Returns the listen key from Binance (POST /api/v3/userDataStream).
    [[nodiscard]] std::string create_listen_key();
    void renew_listen_key(const std::string& listen_key);
    void delete_listen_key(const std::string& listen_key);

protected:
    // Test-injection constructor: accepts a pre-built HTTP transport so unit
    // tests can mock listen-key lifecycle calls without hitting the network.
    // Only usable from code that includes the internal detail headers.
    UserStream(Credentials credentials, std::unique_ptr<::bintrade::rest::detail::HttpTransport> http, WebSocketConfig ws_config = WebSocketConfig{});

    // Dispatch an incoming user-data-stream JSON message to the appropriate
    // typed callback. Protected to allow direct testing without a live connection.
    void dispatch_message(std::string_view raw_json);

    // NVI hook for message dispatch. TypedUserStream<Handler> overrides this
    // to route events to the handler's typed methods via CRTP.
    // The default implementation uses the std::function callbacks.
    virtual void do_dispatch(std::string_view raw_json);

private:
    // Background thread that renews the listen key every 25 minutes.
    // Interruptible via keep_alive_cv_ -- stop() wakes the thread early.
    std::thread keep_alive_thread_;
    std::mutex keep_alive_mutex_;
    std::condition_variable keep_alive_cv_;
    bool stop_keep_alive_{false};  // guarded by keep_alive_mutex_
};

}  // namespace bintrade::ws

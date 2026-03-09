#pragma once

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/config.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/ws/client.hpp>

#include <functional>

namespace bintrade::ws {

using AccountUpdateCallback = std::function<void(const models::AccountInfo& account)>;
using OrderUpdateCallback = std::function<void(const models::Order& order)>;

class UserStream : public Client {
public:
    // rest_config is used for the listen-key lifecycle REST calls
    // (POST/PUT/DELETE /api/v3/userDataStream).
    explicit UserStream(Credentials credentials, RestConfig rest_config = RestConfig{}, WebSocketConfig ws_config = WebSocketConfig{});

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

    // Returns the listen key from Binance (POST /api/v3/userDataStream).
    [[nodiscard]] std::string create_listen_key();
    void renew_listen_key(const std::string& listen_key);
    void delete_listen_key(const std::string& listen_key);

    // Dispatch an incoming user-data-stream JSON message to the appropriate
    // typed callback.
    void dispatch_message(std::string_view raw_json);
};

}  // namespace bintrade::ws

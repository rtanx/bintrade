#pragma once

#include <bintrade/auth/credentials.hpp>
#include <bintrade/models/account.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/ws/client.hpp>

#include <functional>

namespace bintrade::ws {

using AccountUpdateCallback = std::function<void(const models::AccountInfo& account)>;
using OrderUpdateCallback = std::function<void(const models::Order& order)>;

class UserStream : public Client {
public:
    explicit UserStream(Credentials credentials, WebSocketConfig config = WebSocketConfig{});

    void start();
    void keep_alive();
    void stop();

    void set_account_update_callback(AccountUpdateCallback callback);
    void set_order_update_callback(OrderUpdateCallback callback);

private:
    Credentials credentials_;
    std::string listen_key_;
    AccountUpdateCallback account_callback_;
    OrderUpdateCallback order_callback_;
};

}  // namespace bintrade::ws

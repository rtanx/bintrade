#include <bintrade/ws/user_stream.hpp>

#include <stdexcept>

namespace bintrade::ws {

UserStream::UserStream(Credentials credentials, WebSocketConfig config)
    : Client(std::move(config)), credentials_(std::move(credentials)) {}

void UserStream::start() {
    // TODO: POST /api/v3/userDataStream to get listenKey, then connect WS
    throw std::runtime_error("UserStream::start not implemented");
}

void UserStream::keep_alive() {
    // TODO: PUT /api/v3/userDataStream with listenKey
    throw std::runtime_error("UserStream::keep_alive not implemented");
}

void UserStream::stop() {
    // TODO: DELETE /api/v3/userDataStream, then disconnect WS
    disconnect();
}

void UserStream::set_account_update_callback(AccountUpdateCallback callback) {
    account_callback_ = std::move(callback);
}

void UserStream::set_order_update_callback(OrderUpdateCallback callback) {
    order_callback_ = std::move(callback);
}

}  // namespace bintrade::ws

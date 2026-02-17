#include "detail/websocket_client.hpp"

#include <bintrade/ws/client.hpp>

namespace bintrade::ws {

struct Client::Impl {
    detail::WebSocketClient ws;
    MessageCallback on_message;
    ErrorCallback on_error;

    explicit Impl(WebSocketConfig config) : ws(std::move(config)) {}
};

Client::Client(WebSocketConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}

Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

void Client::connect(std::string_view stream_name) {
    impl_->ws.connect(std::string(stream_name));
}

void Client::disconnect() {
    impl_->ws.disconnect();
}

bool Client::is_connected() const {
    return impl_->ws.is_connected();
}

void Client::set_message_callback(MessageCallback callback) {
    impl_->on_message = std::move(callback);
    impl_->ws.set_message_handler([this](std::string_view msg) {
        if (impl_->on_message) {
            impl_->on_message(msg);
        }
    });
}

void Client::set_error_callback(ErrorCallback callback) {
    impl_->on_error = std::move(callback);
}

void Client::send(std::string_view message) {
    impl_->ws.send(message);
}

}  // namespace bintrade::ws

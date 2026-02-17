#include "websocket_client.hpp"

#include <stdexcept>

namespace bintrade::ws::detail {

WebSocketClient::WebSocketClient(WebSocketConfig config) : config_(std::move(config)) {}
WebSocketClient::~WebSocketClient() = default;
WebSocketClient::WebSocketClient(WebSocketClient&&) noexcept = default;
WebSocketClient& WebSocketClient::operator=(WebSocketClient&&) noexcept = default;

void WebSocketClient::connect(const std::string& /*stream_path*/) {
    // TODO: Implement with Boost.Beast WebSocket
    throw std::runtime_error("WebSocketClient::connect not implemented");
}

void WebSocketClient::disconnect() {
    // TODO: Implement graceful close
    connected_ = false;
}

bool WebSocketClient::is_connected() const {
    return connected_;
}

void WebSocketClient::send(std::string_view /*message*/) {
    // TODO: Implement with Boost.Beast
    throw std::runtime_error("WebSocketClient::send not implemented");
}

void WebSocketClient::set_message_handler(MessageHandler handler) {
    on_message_ = std::move(handler);
}

void WebSocketClient::set_error_handler(ErrorHandler handler) {
    on_error_ = std::move(handler);
}

}  // namespace bintrade::ws::detail

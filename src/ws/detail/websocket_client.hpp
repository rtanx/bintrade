#pragma once

#include <bintrade/core/config.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace bintrade::ws::detail {

// Async Boost.Beast WebSocket client over TLS.
//
// Threading model:
//   - connect() launches a dedicated background thread that drives an
//     io_context run loop (1 thread, 1 io_context, 1 stream).
//   - All callbacks are invoked from that I/O thread. Callers MUST NOT
//     perform blocking work inside MessageHandler or ErrorHandler.
//   - send() is safe to call from any thread; it posts to the io_context.
//   - disconnect() is safe to call from any thread.
//
// Reconnection:
//   - On unexpected connection loss the client waits reconnect_interval,
//     doubles the interval up to a maximum of 60 s (with +/-25 % jitter),
//     and retries up to max_reconnect_attempts times.
//   - Each attempt calls ErrorHandler with a descriptive message.
//   - Reconnection stops when disconnect() is called or attempts are
//     exhausted (ErrorHandler is then called with a final message).
class WebSocketClient {
public:
    using MessageHandler = std::function<void(std::string_view)>;
    using ErrorHandler = std::function<void(const std::string&)>;

    explicit WebSocketClient(WebSocketConfig config);
    ~WebSocketClient();

    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;
    WebSocketClient(WebSocketClient&&) noexcept;
    WebSocketClient& operator=(WebSocketClient&&) noexcept;

    // Connect to the given WSS stream path (e.g. "/ws/btcusdt@trade").
    // Blocks until the initial connection attempt either succeeds or fails.
    // After this call is_connected() reflects the connection state.
    // Reconnect logic runs automatically in the background I/O thread.
    void connect(const std::string& stream_path);

    // Gracefully close the WebSocket connection and stop the I/O thread.
    // Safe to call from any thread, including from a callback.
    void disconnect();

    [[nodiscard]] bool is_connected() const noexcept;

    // Post a text message to the server (safe from any thread).
    void send(std::string_view message);

    void set_message_handler(MessageHandler handler);
    void set_error_handler(ErrorHandler handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace bintrade::ws::detail

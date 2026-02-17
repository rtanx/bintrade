#pragma once

#include <bintrade/core/config.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace bintrade::ws::detail {

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

    void connect(const std::string& stream_path);
    void disconnect();
    [[nodiscard]] bool is_connected() const;

    void send(std::string_view message);

    void set_message_handler(MessageHandler handler);
    void set_error_handler(ErrorHandler handler);

private:
    WebSocketConfig config_;
    bool connected_ = false;
    MessageHandler on_message_;
    ErrorHandler on_error_;
};

}  // namespace bintrade::ws::detail

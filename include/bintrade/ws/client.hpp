#pragma once

#include <bintrade/core/config.hpp>
#include <bintrade/core/export.hpp>

#include <functional>
#include <memory>
#include <string_view>
#include <system_error>

namespace bintrade::ws {

using MessageCallback = std::function<void(std::string_view message)>;
using ErrorCallback = std::function<void(const std::error_code& ec, std::string_view message)>;

class BINTRADE_API Client {
public:
    explicit Client(WebSocketConfig config = WebSocketConfig{});
    virtual ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    void connect(std::string_view stream_name);
    void disconnect();
    [[nodiscard]] bool is_connected() const;

    void set_message_callback(MessageCallback callback);
    void set_error_callback(ErrorCallback callback);

    void send(std::string_view message);

protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Deliver a raw message to the registered on-message callback.
    // Protected so test subclasses can inject messages without a live connection.
    void deliver_message(std::string_view msg);

    // Connection NVI hook. Override in test subclasses to suppress real
    // network I/O and capture the stream path. The default implementation
    // delegates to the embedded WebSocketClient.
    virtual void do_connect(std::string_view stream_name);
};

}  // namespace bintrade::ws

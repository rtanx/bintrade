#pragma once

#include <bintrade/core/config.hpp>

#include <functional>
#include <memory>
#include <string_view>
#include <system_error>

namespace bintrade::ws {

using MessageCallback = std::function<void(std::string_view message)>;
using ErrorCallback = std::function<void(const std::error_code& ec, std::string_view message)>;

class Client {
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
};

}  // namespace bintrade::ws

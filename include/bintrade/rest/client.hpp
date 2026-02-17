#pragma once

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/config.hpp>
#include <bintrade/core/types.hpp>

#include <memory>

namespace bintrade::rest {

class Client {
public:
    explicit Client(RestConfig config = RestConfig{});
    Client(RestConfig config, Credentials credentials);

    virtual ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    [[nodiscard]] bool ping();
    [[nodiscard]] Timestamp server_time();

protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace bintrade::rest

#pragma once

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/config.hpp>
#include <bintrade/core/types.hpp>

#include <memory>
#include <string>
#include <unordered_map>

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

    using Params = std::unordered_map<std::string, std::string>;

    /// Unsigned GET — for public/market-data endpoints.
    [[nodiscard]] std::string public_get(const std::string& path, const Params& params = {});

    /// Signed GET — adds timestamp + HMAC-SHA256 signature.
    [[nodiscard]] std::string signed_get(const std::string& path, Params params = {});

    /// Signed POST — adds timestamp + HMAC-SHA256 signature.
    [[nodiscard]] std::string signed_post(const std::string& path, Params params = {});

    /// Signed DELETE — adds timestamp + HMAC-SHA256 signature.
    [[nodiscard]] std::string signed_delete(const std::string& path, Params params = {});
};

}  // namespace bintrade::rest

#pragma once

#include <bintrade/auth/credentials.hpp>
#include <bintrade/core/config.hpp>
#include <bintrade/core/types.hpp>

#include <boost/asio/awaitable.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace bintrade::rest {

// Forward-declare the internal transport interface so the protected
// constructor can accept it without exposing Boost/Beast in public headers.
namespace detail {
class HttpTransport;
}  // namespace detail

class Client {
public:
    explicit Client(RestConfig config = RestConfig{});
    Client(RestConfig config, Credentials credentials);

    virtual ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    // -----------------------------------------------------------------------
    // Synchronous API
    // -----------------------------------------------------------------------
    [[nodiscard]] bool ping();
    [[nodiscard]] Timestamp server_time();

    // Binance rate-limit counters, updated from response headers after every
    // HTTP call.  Values reflect the last server-reported window snapshot.
    // Thread-safe: atomics with relaxed ordering (advisory reads are fine).
    [[nodiscard]] int32_t used_weight_1m() const noexcept;
    [[nodiscard]] int32_t order_count_10s() const noexcept;
    [[nodiscard]] int32_t order_count_1d() const noexcept;

    // -----------------------------------------------------------------------
    // Asynchronous API
    //
    // These coroutines return boost::asio::awaitable<T>.  They are safe to
    // co_await from any Asio executor; internally they dispatch HTTP I/O to
    // the HttpClient's private io_context.
    // -----------------------------------------------------------------------
    [[nodiscard]] boost::asio::awaitable<bool> async_ping();
    [[nodiscard]] boost::asio::awaitable<Timestamp> async_server_time();

protected:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    using Params = std::unordered_map<std::string, std::string>;

    /// Test-injection constructors: accept a pre-built HTTP transport.
    /// Only usable by code that includes the internal detail headers.
    explicit Client(std::unique_ptr<detail::HttpTransport> transport);
    Client(std::unique_ptr<detail::HttpTransport> transport, Credentials credentials);

    // -----------------------------------------------------------------------
    // Synchronous helpers for sub-client implementations.
    // -----------------------------------------------------------------------
    [[nodiscard]] std::string public_get(const std::string& path, const Params& params = {});
    [[nodiscard]] std::string signed_get(const std::string& path, Params params = {});
    [[nodiscard]] std::string signed_post(const std::string& path, Params params = {});
    [[nodiscard]] std::string signed_delete(const std::string& path, Params params = {});

    // -----------------------------------------------------------------------
    // Asynchronous helpers for sub-client implementations.
    // -----------------------------------------------------------------------
    [[nodiscard]] boost::asio::awaitable<std::string> async_public_get(std::string path, Params params = {});
    [[nodiscard]] boost::asio::awaitable<std::string> async_signed_get(std::string path, Params params = {});
    [[nodiscard]] boost::asio::awaitable<std::string> async_signed_post(std::string path, Params params = {});
    [[nodiscard]] boost::asio::awaitable<std::string> async_signed_delete(std::string path, Params params = {});
};

}  // namespace bintrade::rest

#pragma once

#include "http_transport.hpp"

#include <bintrade/core/config.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>

namespace bintrade::rest::detail {

/// Production async HTTP/1.1 transport backed by Boost.Beast over TLS.
///
/// Architecture:
///   - A single io_context runs on a dedicated background thread (io_thread_).
///   - A pool of up to default_pool_size TLS connections is maintained per
///     host:port.  Connections are lazily established and reused across
///     requests (HTTP/1.1 keep-alive).
///   - SSL sessions are cached and reused on reconnect to avoid a full
///     TLS handshake round-trip.
///   - Sync get/post/del block the calling thread by co-spawning the async
///     coroutine on the io_context and waiting on a std::future.
///   - async_get/post/del spawn the coroutine on the io_context's executor
///     and return an asio::awaitable<HttpResponse> that cross-executor
///     co_await is safe to use from any Asio coroutine.
class HttpClient final : public HttpTransport {
public:
    /// Number of pooled TLS connections maintained per host:port.
    static constexpr std::size_t default_pool_size = 4;

    explicit HttpClient(RestConfig config, std::size_t pool_size = default_pool_size);
    ~HttpClient() override;

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = delete;
    HttpClient& operator=(HttpClient&&) = delete;

    void set_api_key(std::string api_key) override;

    // -----------------------------------------------------------------------
    // Synchronous interface -- blocks the calling thread.
    // Internally co_spawns the coroutine and waits on std::future.
    // -----------------------------------------------------------------------
    [[nodiscard]] HttpResponse get(const std::string& path, const Params& params = {}) override;
    [[nodiscard]] HttpResponse post(const std::string& path, const std::string& body = {}, const Params& headers = {}) override;
    [[nodiscard]] HttpResponse del(const std::string& path, const Params& params = {}) override;

    // -----------------------------------------------------------------------
    // Asynchronous interface -- non-blocking, returns an awaitable.
    // The work is spawned on this client's io_context so the Beast streams
    // are always accessed from a single thread.  The awaitable may be
    // co_await-ed from any executor; Asio handles cross-executor resumption.
    // -----------------------------------------------------------------------
    [[nodiscard]] boost::asio::awaitable<HttpResponse> async_get(std::string path, Params params = {}) override;
    [[nodiscard]] boost::asio::awaitable<HttpResponse> async_post(std::string path, std::string body = {}, Params headers = {}) override;
    [[nodiscard]] boost::asio::awaitable<HttpResponse> async_del(std::string path, Params params = {}) override;

    /// Executor bound to this client's io_context.  Use this to co_spawn
    /// client coroutines (e.g. via sync_await) or to run your own work on
    /// the same event loop.
    [[nodiscard]] boost::asio::io_context::executor_type get_executor();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace bintrade::rest::detail

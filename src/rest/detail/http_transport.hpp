#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <string>
#include <unordered_map>

namespace bintrade::rest::detail {

/// Response from an HTTP request.
///
/// Shared value type used by both the production HttpClient and test mocks.
struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

/// Abstract HTTP transport interface.
///
/// Production code uses HttpClient (Beast/HTTPS).  Tests inject a mock via
/// the protected Client(unique_ptr<HttpTransport>) constructor, avoiding all
/// network I/O in unit tests.
///
/// Async interface (async_get/post/del):
///   Default implementations delegate to the synchronous methods so existing
///   mocks and test subclasses require no changes.  HttpClient overrides them
///   with non-blocking Boost.Asio coroutine implementations that spawn work on
///   a persistent io_context thread.
///
/// NOTE: The virtual interface is on the REST path only (~5-500 ms network
/// round-trips).  Virtual dispatch cost (~2-5 ns) is unmeasurable here and
/// does NOT affect the WebSocket hot path.
class HttpTransport {
public:
    using Params = std::unordered_map<std::string, std::string>;

    virtual ~HttpTransport() = default;

    HttpTransport(const HttpTransport&) = delete;
    HttpTransport& operator=(const HttpTransport&) = delete;
    HttpTransport(HttpTransport&&) = delete;
    HttpTransport& operator=(HttpTransport&&) = delete;

    /// Inject API key for the X-MBX-APIKEY header.
    virtual void set_api_key(std::string api_key) = 0;

    // -----------------------------------------------------------------------
    // Synchronous interface (blocking).  Used directly by the current Client
    // hierarchy.  HttpClient implements these by driving async ops via
    // co_spawn + use_future.
    // -----------------------------------------------------------------------

    /// HTTP GET with optional query parameters.
    [[nodiscard]] virtual HttpResponse get(const std::string& path, const Params& params = {}) = 0;

    /// HTTP POST with a body and optional extra headers.
    [[nodiscard]] virtual HttpResponse post(const std::string& path, const std::string& body = {}, const Params& headers = {}) = 0;

    /// HTTP DELETE with optional query parameters.
    [[nodiscard]] virtual HttpResponse del(const std::string& path, const Params& params = {}) = 0;

    // -----------------------------------------------------------------------
    // Asynchronous interface (coroutine-compatible).
    //
    // Default implementations call the synchronous overloads so mocks and
    // test transports need not override them.  HttpClient overrides all three
    // with fully non-blocking Beast coroutine implementations.
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual boost::asio::awaitable<HttpResponse> async_get(std::string path, Params params = {}) { co_return this->get(path, params); }

    [[nodiscard]] virtual boost::asio::awaitable<HttpResponse> async_post(std::string path, std::string body = {}, Params headers = {}) {
        co_return this->post(path, body, headers);
    }

    [[nodiscard]] virtual boost::asio::awaitable<HttpResponse> async_del(std::string path, Params params = {}) { co_return this->del(path, params); }

protected:
    HttpTransport() = default;
};

}  // namespace bintrade::rest::detail

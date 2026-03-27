#include "http_client.hpp"

#include <bintrade/core/error.hpp>

#include <algorithm>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <memory>
#include <mutex>
#include <openssl/ssl.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bintrade::rest::detail {

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using Tcp = asio::ip::tcp;

// ---------------------------------------------------------------------------
// URL parsing helper
// ---------------------------------------------------------------------------
struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string port;

    static ParsedUrl from(const std::string& url) {
        ParsedUrl result;
        auto pos = url.find("://");
        if (pos == std::string::npos) {
            throw std::invalid_argument("Invalid base_url: " + url);
        }
        result.scheme = url.substr(0, pos);

        auto host_start = pos + 3;
        auto port_pos = url.find(':', host_start);
        auto path_pos = url.find('/', host_start);
        if (path_pos == std::string::npos) {
            path_pos = url.size();
        }

        if (port_pos != std::string::npos && port_pos < path_pos) {
            result.host = url.substr(host_start, port_pos - host_start);
            result.port = url.substr(port_pos + 1, path_pos - port_pos - 1);
        } else {
            result.host = url.substr(host_start, path_pos - host_start);
            result.port = (result.scheme == "https") ? "443" : "80";
        }
        return result;
    }
};

// ---------------------------------------------------------------------------
// Query-string builder
// ---------------------------------------------------------------------------
namespace {

std::string build_query_string(const HttpTransport::Params& params) {
    if (params.empty()) {
        return {};
    }
    std::string query;
    for (const auto& [key, value] : params) {
        if (!query.empty()) {
            query += '&';
        }
        query += key;
        query += '=';
        query += value;
    }
    return query;
}

}  // namespace

// ---------------------------------------------------------------------------
// SSL_SESSION custom deleter
// ---------------------------------------------------------------------------
struct SslSessionDeleter {
    void operator()(SSL_SESSION* s) const noexcept {
        if (s != nullptr) {
            SSL_SESSION_free(s);
        }
    }
};
using SslSessionPtr = std::unique_ptr<SSL_SESSION, SslSessionDeleter>;

// ---------------------------------------------------------------------------
// Pooled connection slot
//
// Each slot owns a Beast TLS stream.  Slots in the pool are always accessed
// from the io_context thread (single-threaded event loop), so no additional
// locking is needed on the stream or read_buffer fields.
// ---------------------------------------------------------------------------
struct PooledConnection {
    using SslStream = beast::ssl_stream<beast::tcp_stream>;

    explicit PooledConnection(asio::io_context& ioc, ssl::context& ctx) : stream(std::make_unique<SslStream>(ioc, ctx)) {}

    std::unique_ptr<SslStream> stream;  // non-null when slot is allocated
    beast::flat_buffer read_buffer;     // reused across requests on same connection
    bool in_use = false;
    bool connected = false;
};

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct HttpClient::Impl {
    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------
    explicit Impl(RestConfig cfg, std::size_t ps)
        : config_(std::move(cfg)),
          parsed_url_(ParsedUrl::from(config_.base_url)),
          pool_size_(ps),
          ioc_(1),  // concurrency_hint = 1: optimise for single-threaded loop
          ssl_ctx_(ssl::context::tlsv12_client),
          work_guard_(ioc_.get_executor()),
          io_thread_([this] { ioc_.run(); }) {
        ssl_ctx_.set_default_verify_paths();
        ssl_ctx_.set_verify_mode(config_.verify_ssl ? ssl::verify_peer : ssl::verify_none);
    }

    ~Impl() {
        // 1. Stop accepting new work.
        work_guard_.reset();
        // 2. Force-stop the event loop (any in-flight ops are abandoned).
        //    This is safe in the destructor: callers must ensure no concurrent
        //    requests are pending when destroying HttpClient.
        ioc_.stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        // Pool is cleared by vector destructor after the thread joins.
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    // -----------------------------------------------------------------------
    // Pool management (all called from io_context thread)
    // -----------------------------------------------------------------------

    /// Acquire an idle connection from the pool, connecting it if necessary.
    ///
    /// If the pool is at capacity and all slots are in use, an overflow slot
    /// is appended (pool temporarily exceeds pool_size).  Overflow slots are
    /// removed from the pool in release_connection() instead of being returned.
    asio::awaitable<PooledConnection*> acquire_connection() {
        // Prefer an idle, already-connected slot.
        for (auto& slot : pool_) {
            if (!slot->in_use && slot->connected) {
                slot->in_use = true;
                co_return slot.get();
            }
        }

        // No idle connected slot -- take an idle unconnected slot or create one.
        for (auto& slot : pool_) {
            if (!slot->in_use && !slot->connected) {
                slot->in_use = true;
                co_await do_connect(*slot);
                co_return slot.get();
            }
        }

        // All existing slots are in use -- create an overflow slot.
        auto& new_slot = pool_.emplace_back(std::make_unique<PooledConnection>(ioc_, ssl_ctx_));
        new_slot->in_use = true;
        try {
            co_await do_connect(*new_slot);
        } catch (...) {
            // Remove the failed overflow slot immediately.
            pool_.pop_back();
            throw;
        }
        co_return new_slot.get();
    }

    /// Return a connection to the pool.
    ///
    /// If keep_alive is false, or the pool would exceed pool_size, the slot is
    /// removed and its stream is destroyed.  Otherwise it is marked idle.
    void release_connection(PooledConnection* slot, bool keep_alive) noexcept {
        // auto it = std::find_if(pool_.begin(), pool_.end(), [slot](const auto& s) { return s.get() == slot; });
        // if (it == pool_.end()) {
        //     return;
        // }
        auto it = std::ranges::find_if(pool_, [slot](const auto& s) { return s.get() == slot; });
        if (it == std::ranges::end(pool_)) {
            return;
        }

        // Discard the connection if the server signalled close, or if we're
        // holding more connections than pool_size (overflow slots).
        if (!keep_alive || pool_.size() > pool_size_) {
            pool_.erase(it);
            return;
        }
        (*it)->in_use = false;
    }

    // -----------------------------------------------------------------------
    // Connection establishment (io_context thread only)
    // -----------------------------------------------------------------------

    /// Asynchronously connect and TLS-handshake a PooledConnection.
    /// Reuses a cached SSL_SESSION when available (avoids full handshake).
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters): conn is a pool member on the heap; lifetime guaranteed by Impl::pool_
    asio::awaitable<void> do_connect(PooledConnection& conn) {
        auto& stream = *conn.stream;

        if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed_url_.host.c_str())) {  // NOLINT
            throw NetworkException("Failed to set SNI hostname");
        }

        // Reuse SSL session if cached (reduces TLS round-trips on reconnect).
        auto cache_key = parsed_url_.host + ":" + parsed_url_.port;
        auto sess_it = ssl_session_cache_.find(cache_key);
        if (sess_it != ssl_session_cache_.end()) {
            SSL_set_session(stream.native_handle(), sess_it->second.get());  // NOLINT
        }

        // Resolve (cached by OS resolver; still async to not block ioc thread).
        Tcp::resolver resolver(ioc_);
        const auto endpoints = co_await resolver.async_resolve(parsed_url_.host, parsed_url_.port, asio::use_awaitable);

        // TCP connect.
        beast::get_lowest_layer(stream).expires_after(config_.timeout);
        co_await beast::get_lowest_layer(stream).async_connect(endpoints, asio::use_awaitable);

        // Disable Nagle's algorithm: lower round-trip latency for small requests.
        beast::get_lowest_layer(stream).socket().set_option(Tcp::no_delay(true));

        // TLS handshake.
        beast::get_lowest_layer(stream).expires_after(config_.timeout);
        co_await stream.async_handshake(ssl::stream_base::client, asio::use_awaitable);

        // Cache the negotiated SSL session for future reconnects.
        // SSL_get1_session increments the reference count; we own the pointer.
        if (SSL_SESSION* sess = SSL_get1_session(stream.native_handle())) {  // NOLINT
            ssl_session_cache_[cache_key] = SslSessionPtr(sess);
        }

        conn.connected = true;
    }

    // -----------------------------------------------------------------------
    // Request execution (io_context thread only)
    // -----------------------------------------------------------------------

    /// Execute a single HTTP request on an already-connected stream.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters): conn is a pool member on the heap; lifetime guaranteed by Impl::pool_
    asio::awaitable<HttpResponse> do_request(PooledConnection& conn, http::verb method, std::string target, std::string body,
                                             HttpTransport::Params extra_headers) {
        auto& stream = *conn.stream;

        // Build HTTP/1.1 request.
        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, parsed_url_.host);
        req.set(http::field::user_agent, "bintrade/0.1.0");
        req.set(http::field::accept, "application/json");
        // Signal keep-alive intention explicitly.
        req.set(http::field::connection, "keep-alive");

        {
            std::lock_guard<std::mutex> lk(api_key_mutex_);
            if (!api_key_.empty()) {
                req.set("X-MBX-APIKEY", api_key_);
            }
        }

        for (const auto& [key, value] : extra_headers) {
            req.set(key, value);
        }

        if (!body.empty()) {
            req.set(http::field::content_type, "application/x-www-form-urlencoded");
            req.body() = body;
            req.prepare_payload();
        }

        // Send
        beast::get_lowest_layer(stream).expires_after(config_.timeout);
        co_await http::async_write(stream, req, asio::use_awaitable);

        // Receive
        conn.read_buffer.consume(conn.read_buffer.size());
        http::response<http::string_body> res;
        beast::get_lowest_layer(stream).expires_after(config_.timeout);
        co_await http::async_read(stream, conn.read_buffer, res, asio::use_awaitable);

        // Build response: lower-case header names for case-insensitive consumer lookup.
        HttpResponse response;
        response.status_code = static_cast<int>(res.result_int());
        response.body = std::move(res.body());
        for (const auto& field : res) {
            auto name = std::string(field.name_string());
            std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            response.headers[std::move(name)] = std::string(field.value());
        }

        co_return response;
    }

    /// Full request pipeline: acquire connection, execute request, release.
    ///
    /// If the connection appears stale (initial write or read fails with a
    /// connection-reset/EOF error), the slot is discarded and the request is
    /// retried once on a fresh connection.  A genuine server-side error (4xx
    /// / 5xx) is NOT retried -- only transport-level failures are.
    asio::awaitable<HttpResponse> execute(http::verb method, std::string target, std::string body, HttpTransport::Params extra_headers) {
        constexpr int max_attempts = 2;

        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            PooledConnection* conn = co_await acquire_connection();
            bool was_preconnected = (attempt == 0) && (pool_.size() > 1 || !conn->connected);
            // conn->connected is true at this point (do_connect just ran if needed)

            try {
                auto resp = co_await do_request(*conn, method, target, body, extra_headers);
                // HTTP/1.1: res.keep_alive() checks version + Connection header.
                bool keep = res_keep_alive(resp);
                release_connection(conn, keep);
                co_return resp;
            } catch (const beast::system_error& ec) {
                // Connection reset / EOF on a reused slot -- discard and retry.
                release_connection(conn, false);
                // If this was a fresh connection (not reused), the error is genuine.
                if (!was_preconnected || attempt == max_attempts - 1) {
                    throw NetworkException(ec.what());
                }
                // else: loop and retry with a fresh connection
            } catch (const std::exception& e) {
                release_connection(conn, false);
                throw NetworkException(e.what());
            }
        }
        // Should never reach here; satisfy compiler.
        throw NetworkException("Unexpected: all retry attempts exhausted");
    }

    /// Determine keep-alive from response headers (lowercased by do_request).
    static bool res_keep_alive(const HttpResponse& resp) noexcept {
        auto it = resp.headers.find("connection");
        if (it != resp.headers.end()) {
            return it->second != "close";
        }
        // HTTP/1.1 default: persistent (keep-alive).
        return true;
    }

private:
    RestConfig config_;
    ParsedUrl parsed_url_;
    std::size_t pool_size_;

    // api_key_ is written by set_api_key() from any thread and read from
    // coroutines on the io_context thread.  A mutex guards the cross-thread
    // access.  set_api_key() is a cold-path call (once at startup); the mutex
    // is never contended on the hot path.
    std::string api_key_;
    mutable std::mutex api_key_mutex_;

    // io_context owns the event loop.  work_guard_ prevents ioc_.run() from
    // returning when the pool is momentarily idle.
    asio::io_context ioc_;
    ssl::context ssl_ctx_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
    std::thread io_thread_;

    // SSL session cache: reuse SSL sessions on reconnect to avoid a full
    // TLS handshake.  Keyed by "host:port".  Accessed only from io_context
    // thread (no locking needed).
    std::unordered_map<std::string, SslSessionPtr> ssl_session_cache_;

    // Connection pool: up to pool_size_ persistent TLS connections.  Accessed
    // from coroutines running on ioc_ -- single-threaded, no mutex needed.
    std::vector<std::unique_ptr<PooledConnection>> pool_;

    friend class HttpClient;
};

// ---------------------------------------------------------------------------
// HttpClient public API
// ---------------------------------------------------------------------------
HttpClient::HttpClient(RestConfig config, std::size_t pool_size) : impl_(std::make_unique<Impl>(std::move(config), pool_size)) {}
HttpClient::~HttpClient() = default;

void HttpClient::set_api_key(std::string api_key) {
    std::lock_guard<std::mutex> lk(impl_->api_key_mutex_);
    impl_->api_key_ = std::move(api_key);
}

asio::io_context::executor_type HttpClient::get_executor() {
    return impl_->ioc_.get_executor();
}

// ---------------------------------------------------------------------------
// Async interface (spawns coroutine on io_context, safe for co_await from
// any executor via cross-executor resumption built into Boost.Asio).
// ---------------------------------------------------------------------------

asio::awaitable<HttpResponse> HttpClient::async_get(std::string path, Params params) {
    auto query = build_query_string(params);
    auto target = path;
    if (!query.empty()) {
        target += '?';
        target += query;
    }
    co_return co_await asio::co_spawn(impl_->ioc_.get_executor(), impl_->execute(http::verb::get, std::move(target), {}, {}), asio::use_awaitable);
}

asio::awaitable<HttpResponse> HttpClient::async_post(std::string path, std::string body, Params headers) {
    co_return co_await asio::co_spawn(impl_->ioc_.get_executor(),
                                      impl_->execute(http::verb::post, std::move(path), std::move(body), std::move(headers)), asio::use_awaitable);
}

asio::awaitable<HttpResponse> HttpClient::async_del(std::string path, Params params) {
    auto query = build_query_string(params);
    auto target = path;
    if (!query.empty()) {
        target += '?';
        target += query;
    }
    co_return co_await asio::co_spawn(impl_->ioc_.get_executor(), impl_->execute(http::verb::delete_, std::move(target), {}, {}),
                                      asio::use_awaitable);
}

// ---------------------------------------------------------------------------
// Synchronous convenience wrappers
//
// Each method spawns the async coroutine on the io_context and blocks the
// calling thread via std::future::get().  The io_thread_ is already running
// ioc_.run() so the coroutine makes progress without any additional
// run() call on the calling thread.
// ---------------------------------------------------------------------------

HttpResponse HttpClient::get(const std::string& path, const Params& params) {
    return asio::co_spawn(impl_->ioc_.get_executor(), async_get(path, params), asio::use_future).get();
}

HttpResponse HttpClient::post(const std::string& path, const std::string& body, const Params& headers) {
    return asio::co_spawn(impl_->ioc_.get_executor(), async_post(path, body, headers), asio::use_future).get();
}

HttpResponse HttpClient::del(const std::string& path, const Params& params) {
    return asio::co_spawn(impl_->ioc_.get_executor(), async_del(path, params), asio::use_future).get();
}

}  // namespace bintrade::rest::detail

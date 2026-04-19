#include "websocket_client.hpp"

#include "core/platform/thread_affinity.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/core/logger.hpp>

#include <atomic>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <condition_variable>
#include <mutex>
#include <random>
#include <string>
#include <thread>

namespace bintrade::ws::detail {

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using Tcp = asio::ip::tcp;

// ---------------------------------------------------------------------------
// URL parsing (reused pattern from http_client, but for WSS)
// ---------------------------------------------------------------------------
namespace {

struct ParsedWssUrl {
    std::string host;
    std::string port;

    static ParsedWssUrl from(const std::string& url) {
        // Supports wss://host or wss://host:port
        ParsedWssUrl result;
        auto pos = url.find("://");
        if (pos == std::string::npos) {
            throw std::invalid_argument("Invalid WebSocket URL: " + url);
        }
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
            result.port = "443";
        }
        return result;
    }
};

// Jitter: return a duration in [base * 0.75, base * 1.25].
std::chrono::milliseconds jitter(std::chrono::milliseconds base) {
    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(0.75, 1.25);
    auto ms = static_cast<long long>(static_cast<double>(base.count()) * dist(rng));
    return std::chrono::milliseconds(ms);
}

}  // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct WebSocketClient::Impl {
    WebSocketConfig config;
    ParsedWssUrl parsed_url;

    // Stream and I/O context -- owned on the I/O thread after connect().
    asio::io_context ioc{1};
    ssl::context ssl_ctx{ssl::context::tlsv12_client};

    using WsStream = websocket::stream<beast::ssl_stream<beast::tcp_stream>>;
    std::unique_ptr<WsStream> ws;

    // Callbacks (set before connect()).
    MessageHandler on_message;
    ErrorHandler on_error;

    // State
    std::atomic<bool> connected{false};
    std::atomic<bool> should_stop{false};
    std::string current_path;

    // The I/O thread drives ioc.run().
    std::thread io_thread;

    // Used to wait for the first connect() attempt to complete.
    std::mutex connect_mutex;
    std::condition_variable connect_cv;
    bool connect_done{false};
    std::string connect_error;

    // Read buffer -- reused across reads to avoid allocation.
    beast::flat_buffer read_buffer;

    explicit Impl(WebSocketConfig cfg) : config(std::move(cfg)), parsed_url(ParsedWssUrl::from(config.base_url)) {
        ssl_ctx.set_default_verify_paths();
        ssl_ctx.set_verify_mode(ssl::verify_peer);
    }

    // Notify the waiting connect() call (success or failure).
    void notify_connect(std::string error_msg) {
        {
            std::scoped_lock lk(connect_mutex);
            connect_done = true;
            connect_error = std::move(error_msg);
        }
        connect_cv.notify_one();
    }

    // Open a new stream and perform TCP resolve + connect + SSL + WS handshake.
    // Called from the I/O thread only.
    void do_connect() {
        try {
            ws = std::make_unique<WsStream>(ioc, ssl_ctx);

            if (!SSL_set_tlsext_host_name(  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
                    ws->next_layer().native_handle(), parsed_url.host.c_str())) {
                throw NetworkException("Failed to set SNI hostname");
            }

            Tcp::resolver resolver(ioc);
            const auto endpoints = resolver.resolve(parsed_url.host, parsed_url.port);

            beast::get_lowest_layer(*ws).expires_after(config.ping_interval);
            beast::get_lowest_layer(*ws).connect(endpoints);

            // Disable Nagle's algorithm for lower round-trip latency.
            beast::get_lowest_layer(*ws).socket().set_option(Tcp::no_delay(true));

            beast::get_lowest_layer(*ws).expires_after(config.ping_interval);
            ws->next_layer().handshake(ssl::stream_base::client);

            // Disable Beast's built-in timeout -- we handle pings manually.
            beast::get_lowest_layer(*ws).expires_never();
            ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

            // Identify ourselves in the HTTP upgrade request.
            ws->set_option(websocket::stream_base::decorator(
                [](websocket::request_type& req) { req.set(boost::beast::http::field::user_agent, "bintrade/0.1.0"); }));

            ws->handshake(parsed_url.host, current_path);
            connected.store(true, std::memory_order_release);
            bintrade::logger()->info("WS connected to {}:{}{}", parsed_url.host, parsed_url.port, current_path);

        } catch (const std::exception& e) {
            connected.store(false, std::memory_order_release);
            bintrade::logger()->warn("WS connect failed: {}", e.what());
            notify_connect(e.what());
            return;
        }

        notify_connect({});  // success

        // Kick off the async read loop and ping timer.
        do_read();
        do_ping_timer();
    }

    // Async read loop -- the "recursion" is through an async callback chain,
    // not actual stack recursion. Each call to do_read() posts an async op that
    // completes later on a new stack frame inside io_context::run().
    void do_read() {  // NOLINT(misc-no-recursion)
        if (should_stop.load(std::memory_order_acquire)) {
            return;
        }

        // NOLINTNEXTLINE(misc-no-recursion)
        ws->async_read(read_buffer, [this](beast::error_code ec, std::size_t bytes_transferred) { handle_read(ec, bytes_transferred); });
    }

    void handle_read(beast::error_code ec, std::size_t /*bytes*/) {  // NOLINT(misc-no-recursion)
        if (should_stop.load(std::memory_order_acquire)) {
            return;
        }
        if (ec) {
            connected.store(false, std::memory_order_release);
            bintrade::logger()->warn("WS read error: {}", ec.message());
            if (on_error) {
                on_error("Read error: " + ec.message());
            }
            if (!should_stop.load(std::memory_order_acquire)) {
                attempt_reconnect(0, config.reconnect_interval);
            }
            return;
        }

        if (ws->got_text()) {
            auto msg = beast::buffers_to_string(read_buffer.data());
            read_buffer.consume(read_buffer.size());
            bintrade::logger()->trace("WS recv {} bytes", msg.size());
            if (on_message) {
                on_message(msg);
            }
        } else {
            // Binary or ping/pong -- Beast handles pong automatically for ping;
            // just consume the buffer.
            read_buffer.consume(read_buffer.size());
        }

        do_read();
    }

    // Periodic ping to keep the connection alive and detect silent drops.
    std::unique_ptr<asio::steady_timer> ping_timer;

    void do_ping_timer() {
        ping_timer = std::make_unique<asio::steady_timer>(ioc, config.ping_interval);
        ping_timer->async_wait([this](beast::error_code ec) {
            if (ec || should_stop.load(std::memory_order_acquire)) {
                return;
            }
            if (connected.load(std::memory_order_acquire)) {
                beast::error_code ping_ec;
                ws->ping({}, ping_ec);
                if (ping_ec) {
                    connected.store(false, std::memory_order_release);
                    bintrade::logger()->warn("WS ping failed: {}", ping_ec.message());
                    if (on_error) {
                        on_error("Ping failed: " + ping_ec.message());
                    }
                    attempt_reconnect(0, config.reconnect_interval);
                    return;
                }
            }
            do_ping_timer();
        });
    }

    // Exponential backoff with jitter.  attempt 0 -> reconnect_interval,
    // doubles each time, capped at 60 s.
    void attempt_reconnect(int attempt, std::chrono::seconds delay) {
        if (should_stop.load(std::memory_order_acquire)) {
            return;
        }
        if (attempt >= config.max_reconnect_attempts) {
            bintrade::logger()->error("WS max reconnect attempts ({}) reached -- giving up", config.max_reconnect_attempts);
            if (on_error) {
                on_error("Max reconnect attempts reached -- giving up");
            }
            return;
        }

        auto jittered = jitter(std::chrono::duration_cast<std::chrono::milliseconds>(delay));

        auto timer = std::make_shared<asio::steady_timer>(ioc, jittered);
        timer->async_wait([this, attempt, delay, timer](beast::error_code ec) {
            if (ec || should_stop.load(std::memory_order_acquire)) {
                return;
            }
            bintrade::logger()->info("WS reconnecting (attempt {}/{})", attempt + 1, config.max_reconnect_attempts);
            if (on_error) {
                on_error("Reconnecting (attempt " + std::to_string(attempt + 1) + ")...");
            }
            // Re-use the same path.
            try {
                ws = std::make_unique<WsStream>(ioc, ssl_ctx);

                if (!SSL_set_tlsext_host_name(  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
                        ws->next_layer().native_handle(), parsed_url.host.c_str())) {
                    throw NetworkException("SNI failed on reconnect");
                }

                Tcp::resolver resolver(ioc);
                const auto endpoints = resolver.resolve(parsed_url.host, parsed_url.port);

                beast::get_lowest_layer(*ws).expires_after(config.ping_interval);
                beast::get_lowest_layer(*ws).connect(endpoints);

                beast::get_lowest_layer(*ws).expires_after(config.ping_interval);
                ws->next_layer().handshake(ssl::stream_base::client);

                beast::get_lowest_layer(*ws).expires_never();
                ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
                ws->set_option(websocket::stream_base::decorator(
                    [](websocket::request_type& req) { req.set(boost::beast::http::field::user_agent, "bintrade/0.1.0"); }));

                ws->handshake(parsed_url.host, current_path);
                connected.store(true, std::memory_order_release);
                bintrade::logger()->info("WS reconnected to {}:{}{}", parsed_url.host, parsed_url.port, current_path);
                read_buffer.clear();
                do_read();
                do_ping_timer();

            } catch (const std::exception& e) {
                connected.store(false, std::memory_order_release);
                bintrade::logger()->warn("WS reconnect attempt {} failed: {}", attempt + 1, e.what());
                if (on_error) {
                    on_error(std::string("Reconnect attempt failed: ") + e.what());
                }
                using namespace std::chrono_literals;
                auto next_delay = std::chrono::seconds(std::min(static_cast<long long>(delay.count()) * 2, 60LL));
                attempt_reconnect(attempt + 1, next_delay);
            }
        });
    }

    void close_stream() const {
        if (!ws) {
            return;
        }
        beast::error_code ec;
        ws->close(websocket::close_code::normal, ec);
        // Ignore close errors -- the stream may already be broken.
    }
};

// ---------------------------------------------------------------------------
// WebSocketClient public API
// ---------------------------------------------------------------------------
WebSocketClient::WebSocketClient(WebSocketConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}

WebSocketClient::~WebSocketClient() {
    try {
        disconnect();
    } catch (...) {  // NOLINT(bugprone-empty-catch)
        // Destructor must not throw. disconnect() failures are non-fatal.
    }
}

WebSocketClient::WebSocketClient(WebSocketClient&&) noexcept = default;
WebSocketClient& WebSocketClient::operator=(WebSocketClient&&) noexcept = default;

void WebSocketClient::connect(const std::string& stream_path) {
    impl_->should_stop.store(false, std::memory_order_release);
    impl_->connect_done = false;
    impl_->connect_error.clear();
    impl_->current_path = stream_path;

    // Launch the I/O thread. It will call do_connect() which eventually
    // calls notify_connect() to unblock the wait below.
    impl_->io_thread = std::thread([this]() {
        bintrade::platform::pin_thread_to_core(impl_->config.io_core_id);
        impl_->do_connect();
        impl_->ioc.run();
    });

    // Block until the first connect attempt resolves.
    {
        std::unique_lock<std::mutex> lk(impl_->connect_mutex);
        impl_->connect_cv.wait(lk, [this] { return impl_->connect_done; });
    }

    if (!impl_->connect_error.empty()) {
        impl_->should_stop.store(true, std::memory_order_release);
        if (impl_->io_thread.joinable()) {
            impl_->io_thread.join();
        }
        throw NetworkException("WebSocket connect failed: " + impl_->connect_error);
    }
}

void WebSocketClient::disconnect() {
    if (!impl_) {
        return;
    }
    bintrade::logger()->info("WS disconnecting");
    impl_->should_stop.store(true, std::memory_order_release);
    impl_->connected.store(false, std::memory_order_release);

    // Cancel the ping timer so it does not fire after the stream is closed.
    if (impl_->ping_timer) {
        impl_->ping_timer->cancel();
    }

    // Post close to the I/O thread rather than calling it from outside.
    asio::post(impl_->ioc, [this]() {
        impl_->close_stream();
        impl_->ioc.stop();
    });

    if (impl_->io_thread.joinable()) {
        impl_->io_thread.join();
    }
}

bool WebSocketClient::is_connected() const noexcept {
    return impl_ && impl_->connected.load(std::memory_order_acquire);
}

void WebSocketClient::send(std::string_view message) {
    if (!is_connected()) {
        throw NetworkException("WebSocket send failed: not connected");
    }
    // Copy message for the lambda capture since string_view lifetime is caller's.
    std::string msg{message};
    asio::post(impl_->ioc, [this, msg = std::move(msg)]() {
        if (!impl_->connected.load(std::memory_order_acquire)) {
            return;
        }
        beast::error_code ec;
        impl_->ws->write(asio::buffer(msg), ec);
        if (ec && impl_->on_error) {
            impl_->on_error("Send error: " + ec.message());
        }
    });
}

void WebSocketClient::set_message_handler(MessageHandler handler) {
    impl_->on_message = std::move(handler);
}

void WebSocketClient::set_error_handler(ErrorHandler handler) {
    impl_->on_error = std::move(handler);
}

}  // namespace bintrade::ws::detail

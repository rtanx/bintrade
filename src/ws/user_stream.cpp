#include "detail/ws_parse.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/ws/user_stream.hpp>

// For listen-key lifecycle REST calls we reuse the internal HTTP client
// directly rather than depending on the public rest:: layer.
// The src/ directory is on the private include path so these are reachable.
#include "rest/detail/http_client.hpp"
#include "rest/detail/json_parse.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <utility>

namespace bintrade::ws {

UserStream::UserStream(Credentials credentials, RestConfig rest_config, WebSocketConfig ws_config)
    : Client(std::move(ws_config)), credentials_(std::move(credentials)), rest_config_(std::move(rest_config)) {}

UserStream::UserStream(Credentials credentials, std::unique_ptr<rest::detail::HttpTransport> http, WebSocketConfig ws_config)
    : Client(std::move(ws_config)), credentials_(std::move(credentials)), rest_http_(std::move(http)) {}

// ---------------------------------------------------------------------------
// Listen-key REST helpers
// ---------------------------------------------------------------------------
std::string UserStream::create_listen_key() {
    rest::detail::HttpTransport* http = rest_http_.get();
    std::unique_ptr<rest::detail::HttpClient> owned_http;
    if (http == nullptr) {
        owned_http = std::make_unique<rest::detail::HttpClient>(rest_config_);
        owned_http->set_api_key(std::string(credentials_.api_key()));
        http = owned_http.get();
    } else {
        http->set_api_key(std::string(credentials_.api_key()));
    }

    auto resp = http->post("/api/v3/userDataStream");
    auto json = rest::detail::parse_response(resp.status_code, resp.body);
    return json.value("listenKey", "");
}

void UserStream::renew_listen_key(const std::string& listen_key) {
    rest::detail::HttpTransport* http = rest_http_.get();
    std::unique_ptr<rest::detail::HttpClient> owned_http;
    if (http == nullptr) {
        owned_http = std::make_unique<rest::detail::HttpClient>(rest_config_);
        owned_http->set_api_key(std::string(credentials_.api_key()));
        http = owned_http.get();
    } else {
        http->set_api_key(std::string(credentials_.api_key()));
    }

    // The Binance keep-alive endpoint is PUT with listenKey in the body.
    auto resp = http->post("/api/v3/userDataStream?listenKey=" + listen_key);
    // A 200 with an empty body is the success response -- just check the status.
    if (resp.status_code >= 400) {
        rest::detail::parse_response(resp.status_code, resp.body);  // throws
    }
}

void UserStream::delete_listen_key(const std::string& listen_key) {
    rest::detail::HttpTransport* http = rest_http_.get();
    std::unique_ptr<rest::detail::HttpClient> owned_http;
    if (http == nullptr) {
        owned_http = std::make_unique<rest::detail::HttpClient>(rest_config_);
        owned_http->set_api_key(std::string(credentials_.api_key()));
        http = owned_http.get();
    } else {
        http->set_api_key(std::string(credentials_.api_key()));
    }

    auto resp = http->del("/api/v3/userDataStream", {{"listenKey", listen_key}});
    if (resp.status_code >= 400) {
        rest::detail::parse_response(resp.status_code, resp.body);  // throws
    }
}

// ---------------------------------------------------------------------------
// Message dispatch
// ---------------------------------------------------------------------------
void UserStream::dispatch_message(std::string_view raw_json) {
    try {
        auto j = nlohmann::json::parse(raw_json);
        const auto event_type = j.value("e", "");

        if (event_type == "executionReport") {
            if (order_callback_) {
                order_callback_(detail::parse_order_update(j));
            }
        } else if (event_type == "outboundAccountPosition") {
            if (account_callback_) {
                account_callback_(detail::parse_account_update(j));
            }
        }
        // "balanceUpdate" and "listStatus" events are intentionally not
        // dispatched at this stage; they can be added as needed.
    } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
        // Non-fatal: discard unparseable messages.
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void UserStream::start() {
    if (!credentials_.has_credentials()) {
        throw AuthenticationException("UserStream::start requires valid credentials");
    }

    listen_key_ = create_listen_key();
    if (listen_key_.empty()) {
        throw ApiException(0, "Empty listenKey returned by Binance");
    }

    set_message_callback([this](std::string_view msg) { dispatch_message(msg); });

    connect("/ws/" + listen_key_);

    // Renew the listen key every 25 minutes. The Binance listen key expires
    // after 60 minutes of inactivity; 25 minutes gives a comfortable margin.
    // condition_variable::wait_until with a predicate enables clean shutdown:
    // stop() sets stop_keep_alive_ under the mutex and notifies, waking the
    // thread immediately instead of waiting out the full 25-minute interval.
    stop_keep_alive_ = false;
    keep_alive_thread_ = std::thread([this] {
        static constexpr auto k_interval = std::chrono::minutes{25};
        std::unique_lock<std::mutex> lk(keep_alive_mutex_);
        while (!stop_keep_alive_) {
            const auto wakeup = std::chrono::steady_clock::now() + k_interval;
            if (!keep_alive_cv_.wait_until(lk, wakeup, [this] { return stop_keep_alive_; })) {
                // Timed out: renew outside the lock to avoid blocking stop().
                lk.unlock();
                keep_alive();
                lk.lock();
            }
        }
    });
}

void UserStream::keep_alive() {
    if (listen_key_.empty()) {
        return;
    }
    renew_listen_key(listen_key_);
}

void UserStream::stop() {
    // Stop the keep-alive thread first to prevent a race on listen_key_.
    {
        std::scoped_lock lk(keep_alive_mutex_);
        stop_keep_alive_ = true;
    }
    keep_alive_cv_.notify_one();
    if (keep_alive_thread_.joinable()) {
        keep_alive_thread_.join();
    }

    if (!listen_key_.empty()) {
        try {
            delete_listen_key(listen_key_);
        } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
            // Best-effort: disconnect regardless.
        }
        listen_key_.clear();
    }
    disconnect();
}

void UserStream::set_account_update_callback(AccountUpdateCallback callback) {
    account_callback_ = std::move(callback);
}

void UserStream::set_order_update_callback(OrderUpdateCallback callback) {
    order_callback_ = std::move(callback);
}

}  // namespace bintrade::ws

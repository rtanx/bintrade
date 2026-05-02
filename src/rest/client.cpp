#include "detail/http_client.hpp"
#include "detail/http_transport.hpp"
#include "detail/json_parse.hpp"

#include <bintrade/auth/signer.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/rest/client.hpp>

#include <atomic>
#include <boost/asio/awaitable.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace bintrade::rest {

namespace asio = boost::asio;

// ---------------------------------------------------------------------------
// Client::Impl -- holds the HTTP transport + optional auth components
// ---------------------------------------------------------------------------
struct Client::Impl {
    std::unique_ptr<detail::HttpTransport> http;
    std::unique_ptr<Credentials> credentials;
    std::unique_ptr<Signer> signer;

    // Rate-limit counters: updated from Binance response headers after each
    // request.  Header names are lowercased by HttpClient. relaxed ordering
    // is sufficient -- callers use these as advisory hints, not hard fences.
    std::atomic<int32_t> used_weight_1m{0};
    std::atomic<int32_t> order_count_10s{0};
    std::atomic<int32_t> order_count_1d{0};

    void update_rate_limits(const detail::HttpResponse& resp) noexcept {
        auto try_parse = [](const std::string& v) noexcept -> int32_t {
            try {
                return std::stoi(v);
            } catch (const std::exception&) {
                return -1;
            }
        };
        auto it = resp.headers.find("x-mbx-used-weight-1m");
        if (it != resp.headers.end()) {
            used_weight_1m.store(try_parse(it->second), std::memory_order_relaxed);
        }
        it = resp.headers.find("x-mbx-order-count-10s");
        if (it != resp.headers.end()) {
            order_count_10s.store(try_parse(it->second), std::memory_order_relaxed);
        }
        it = resp.headers.find("x-mbx-order-count-1d");
        if (it != resp.headers.end()) {
            order_count_1d.store(try_parse(it->second), std::memory_order_relaxed);
        }
    }

    explicit Impl(RestConfig config) : http(std::make_unique<detail::HttpClient>(std::move(config))) {}

    Impl(RestConfig config, Credentials creds)
        : http(std::make_unique<detail::HttpClient>(std::move(config))),
          credentials(std::make_unique<Credentials>(std::move(creds))),
          signer(std::make_unique<Signer>(*credentials)) {
        http->set_api_key(std::string(credentials->api_key()));
    }

    explicit Impl(std::unique_ptr<detail::HttpTransport> transport) : http(std::move(transport)) {}

    Impl(std::unique_ptr<detail::HttpTransport> transport, Credentials creds)
        : http(std::move(transport)), credentials(std::make_unique<Credentials>(std::move(creds))), signer(std::make_unique<Signer>(*credentials)) {
        http->set_api_key(std::string(credentials->api_key()));
    }

    /// Add timestamp + signature to params using the Signer.
    void sign_params(std::unordered_map<std::string, std::string>& params) const {
        if (!signer) {
            throw AuthenticationException("No credentials provided for signed request");
        }
        params = signer->sign_parameters(std::move(params));
    }
};

// ---------------------------------------------------------------------------
// Constructors / Destructors / Move
// ---------------------------------------------------------------------------
Client::Client(RestConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}

Client::Client(RestConfig config, Credentials credentials) : impl_(std::make_unique<Impl>(std::move(config), std::move(credentials))) {}

Client::Client(std::unique_ptr<detail::HttpTransport> transport) : impl_(std::make_unique<Impl>(std::move(transport))) {}

Client::Client(std::unique_ptr<detail::HttpTransport> transport, Credentials credentials)
    : impl_(std::make_unique<Impl>(std::move(transport), std::move(credentials))) {}

Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

// ---------------------------------------------------------------------------
// Public sync API -- basic endpoints
// ---------------------------------------------------------------------------
bool Client::ping() {
    auto resp = impl_->http->get("/api/v3/ping");
    impl_->update_rate_limits(resp);
    return resp.status_code == 200;
}

Timestamp Client::server_time() {
    auto resp = impl_->http->get("/api/v3/time");
    impl_->update_rate_limits(resp);
    auto json = detail::parse_response(resp.status_code, resp.body);
    auto ms = json.value("serverTime", int64_t{0});
    return Timestamp(std::chrono::milliseconds(ms));
}

int32_t Client::used_weight_1m() const noexcept {
    return impl_->used_weight_1m.load(std::memory_order_relaxed);
}

int32_t Client::order_count_10s() const noexcept {
    return impl_->order_count_10s.load(std::memory_order_relaxed);
}

int32_t Client::order_count_1d() const noexcept {
    return impl_->order_count_1d.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Protected sync helpers -- auth wiring for subclasses
// ---------------------------------------------------------------------------
std::string Client::public_get(const std::string& path, const Params& params) {
    auto resp = impl_->http->get(path, params);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    return resp.body;
}

std::string Client::signed_get(const std::string& path, Params params) {
    impl_->sign_params(params);
    auto resp = impl_->http->get(path, params);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    return resp.body;
}

std::string Client::signed_post(const std::string& path, Params params) {
    impl_->sign_params(params);

    // Build form body from signed params
    std::string body;
    for (const auto& [key, value] : params) {
        if (!body.empty()) {
            body += '&';
        }
        body += key;
        body += '=';
        body += value;
    }

    auto resp = impl_->http->post(path, body);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    return resp.body;
}

std::string Client::signed_delete(const std::string& path, Params params) {
    impl_->sign_params(params);
    auto resp = impl_->http->del(path, params);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    return resp.body;
}

// ---------------------------------------------------------------------------
// Public async API
// ---------------------------------------------------------------------------

asio::awaitable<bool> Client::async_ping() {
    auto resp = co_await impl_->http->async_get("/api/v3/ping");
    impl_->update_rate_limits(resp);
    co_return resp.status_code == 200;
}

asio::awaitable<Timestamp> Client::async_server_time() {
    auto resp = co_await impl_->http->async_get("/api/v3/time");
    impl_->update_rate_limits(resp);
    auto json = detail::parse_response(resp.status_code, resp.body);
    auto ms = json.value("serverTime", int64_t{0});
    co_return Timestamp(std::chrono::milliseconds(ms));
}

// ---------------------------------------------------------------------------
// Protected async helpers -- auth wiring for sub-client async methods
// ---------------------------------------------------------------------------

asio::awaitable<std::string> Client::async_public_get(std::string path, Params params) {
    auto resp = co_await impl_->http->async_get(path, params);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    co_return resp.body;
}

asio::awaitable<std::string> Client::async_signed_get(std::string path, Params params) {
    impl_->sign_params(params);
    auto resp = co_await impl_->http->async_get(path, params);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    co_return resp.body;
}

asio::awaitable<std::string> Client::async_signed_post(std::string path, Params params) {
    impl_->sign_params(params);

    std::string body;
    for (const auto& [key, value] : params) {
        if (!body.empty()) {
            body += '&';
        }
        body += key;
        body += '=';
        body += value;
    }

    auto resp = co_await impl_->http->async_post(path, body);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    co_return resp.body;
}

asio::awaitable<std::string> Client::async_signed_delete(std::string path, Params params) {
    impl_->sign_params(params);
    auto resp = co_await impl_->http->async_del(path, params);
    impl_->update_rate_limits(resp);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    co_return resp.body;
}

}  // namespace bintrade::rest

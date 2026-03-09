#include "detail/http_client.hpp"
#include "detail/json_parse.hpp"

#include <bintrade/auth/signer.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/rest/client.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace bintrade::rest {

// ---------------------------------------------------------------------------
// Client::Impl — holds the HTTP transport + optional auth components
// ---------------------------------------------------------------------------
struct Client::Impl {
    detail::HttpClient http;
    std::unique_ptr<Credentials> credentials;
    std::unique_ptr<Signer> signer;

    explicit Impl(RestConfig config) : http(std::move(config)) {}

    Impl(RestConfig config, Credentials creds)
        : http(std::move(config)), credentials(std::make_unique<Credentials>(std::move(creds))), signer(std::make_unique<Signer>(*credentials)) {
        // Inject API key into HTTP client for X-MBX-APIKEY header
        http.set_api_key(std::string(credentials->api_key()));
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

Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

// ---------------------------------------------------------------------------
// Public API — basic endpoints
// ---------------------------------------------------------------------------
bool Client::ping() {
    auto resp = impl_->http.get("/api/v3/ping");
    return resp.status_code == 200;
}

Timestamp Client::server_time() {
    auto resp = impl_->http.get("/api/v3/time");
    auto json = detail::parse_response(resp.status_code, resp.body);
    auto ms = json.value("serverTime", int64_t{0});
    return Timestamp(std::chrono::milliseconds(ms));
}

// ---------------------------------------------------------------------------
// Protected helpers — auth wiring for subclasses
// ---------------------------------------------------------------------------
std::string Client::public_get(const std::string& path, const Params& params) {
    auto resp = impl_->http.get(path, params);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    return resp.body;
}

std::string Client::signed_get(const std::string& path, Params params) {
    impl_->sign_params(params);
    auto resp = impl_->http.get(path, params);
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

    auto resp = impl_->http.post(path, body);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    return resp.body;
}

std::string Client::signed_delete(const std::string& path, Params params) {
    impl_->sign_params(params);
    auto resp = impl_->http.del(path, params);
    if (resp.status_code >= 400) {
        detail::parse_response(resp.status_code, resp.body);  // throws
    }
    return resp.body;
}

}  // namespace bintrade::rest

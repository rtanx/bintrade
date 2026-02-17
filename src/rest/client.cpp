#include "detail/http_client.hpp"

#include <bintrade/rest/client.hpp>

#include <stdexcept>

namespace bintrade::rest {

struct Client::Impl {
    detail::HttpClient http;
    std::unique_ptr<Credentials> credentials;

    explicit Impl(RestConfig config) : http(std::move(config)) {}
    Impl(RestConfig config, Credentials creds)
        : http(std::move(config)), credentials(std::make_unique<Credentials>(std::move(creds))) {}
};

Client::Client(RestConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}

Client::Client(RestConfig config, Credentials credentials)
    : impl_(std::make_unique<Impl>(std::move(config), std::move(credentials))) {}

Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

bool Client::ping() {
    // TODO: GET /api/v3/ping
    throw std::runtime_error("Client::ping not implemented");
}

Timestamp Client::server_time() {
    // TODO: GET /api/v3/time
    throw std::runtime_error("Client::server_time not implemented");
}

}  // namespace bintrade::rest

#include "http_client.hpp"

#include <stdexcept>

namespace bintrade::rest::detail {

HttpClient::HttpClient(RestConfig config) : config_(std::move(config)) {}
HttpClient::~HttpClient() = default;
HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

HttpClient::Response
HttpClient::get(const std::string& /*path*/,
                const std::unordered_map<std::string, std::string>& /*params*/) {
    // TODO: Implement with Boost.Beast
    throw std::runtime_error("HttpClient::get not implemented");
}

HttpClient::Response
HttpClient::post(const std::string& /*path*/, const std::string& /*body*/,
                 const std::unordered_map<std::string, std::string>& /*headers*/) {
    // TODO: Implement with Boost.Beast
    throw std::runtime_error("HttpClient::post not implemented");
}

HttpClient::Response
HttpClient::del(const std::string& /*path*/,
                const std::unordered_map<std::string, std::string>& /*params*/) {
    // TODO: Implement with Boost.Beast
    throw std::runtime_error("HttpClient::del not implemented");
}

}  // namespace bintrade::rest::detail

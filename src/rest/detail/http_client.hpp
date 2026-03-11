#pragma once

#include "http_transport.hpp"

#include <bintrade/core/config.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace bintrade::rest::detail {

/// Production HTTP transport backed by Boost.Beast over TLS.
class HttpClient final : public HttpTransport {
public:
    explicit HttpClient(RestConfig config);
    ~HttpClient() override;

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = delete;
    HttpClient& operator=(HttpClient&&) = delete;

    void set_api_key(std::string api_key) override;

    [[nodiscard]] HttpResponse get(const std::string& path, const std::unordered_map<std::string, std::string>& params = {}) override;

    [[nodiscard]] HttpResponse post(const std::string& path, const std::string& body = {},
                                    const std::unordered_map<std::string, std::string>& headers = {}) override;

    [[nodiscard]] HttpResponse del(const std::string& path, const std::unordered_map<std::string, std::string>& params = {}) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace bintrade::rest::detail

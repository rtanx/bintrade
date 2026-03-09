#pragma once

#include <bintrade/core/config.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace bintrade::rest::detail {

class HttpClient {
public:
    explicit HttpClient(RestConfig config);
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;

    struct Response {
        int status_code = 0;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
    };

    /// Set the API key for X-MBX-APIKEY header injection.
    void set_api_key(std::string api_key);

    /// HTTP GET with optional query params.
    [[nodiscard]] Response get(const std::string& path, const std::unordered_map<std::string, std::string>& params = {});

    /// HTTP POST with body and optional extra headers.
    [[nodiscard]] Response post(const std::string& path, const std::string& body = {},
                                const std::unordered_map<std::string, std::string>& headers = {});

    /// HTTP DELETE with optional query params.
    [[nodiscard]] Response del(const std::string& path, const std::unordered_map<std::string, std::string>& params = {});

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace bintrade::rest::detail

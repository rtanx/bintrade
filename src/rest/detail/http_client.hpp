#pragma once

#include <bintrade/core/config.hpp>

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

    [[nodiscard]] Response get(const std::string& path,
                               const std::unordered_map<std::string, std::string>& params = {});

    [[nodiscard]] Response post(const std::string& path, const std::string& body = {},
                                const std::unordered_map<std::string, std::string>& headers = {});

    [[nodiscard]] Response del(const std::string& path,
                               const std::unordered_map<std::string, std::string>& params = {});

private:
    RestConfig config_;
};

}  // namespace bintrade::rest::detail

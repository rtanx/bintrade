#pragma once

#include <string>
#include <unordered_map>

namespace bintrade::rest::detail {

/// Response from an HTTP request.
///
/// Shared value type used by both the production HttpClient and test mocks.
struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

/// Abstract HTTP transport interface.
///
/// Production code uses HttpClient (Beast/HTTPS).  Tests inject a mock via
/// the protected Client(unique_ptr<HttpTransport>) constructor, avoiding all
/// network I/O in unit tests.
///
/// NOTE: This virtual interface is intentionally on the REST path only, which
/// already involves blocking network round-trips (~5-500 ms).  The virtual
/// dispatch cost (~2-5 ns) is unmeasurable in this context and does NOT
/// affect the WebSocket hot path.
class HttpTransport {
public:
    virtual ~HttpTransport() = default;

    HttpTransport(const HttpTransport&) = delete;
    HttpTransport& operator=(const HttpTransport&) = delete;
    HttpTransport(HttpTransport&&) = delete;
    HttpTransport& operator=(HttpTransport&&) = delete;

    /// Inject API key for the X-MBX-APIKEY header.
    virtual void set_api_key(std::string api_key) = 0;

    /// HTTP GET with optional query parameters.
    [[nodiscard]] virtual HttpResponse get(const std::string& path, const std::unordered_map<std::string, std::string>& params = {}) = 0;

    /// HTTP POST with a body and optional extra headers.
    [[nodiscard]] virtual HttpResponse post(const std::string& path, const std::string& body = {},
                                            const std::unordered_map<std::string, std::string>& headers = {}) = 0;

    /// HTTP DELETE with optional query parameters.
    [[nodiscard]] virtual HttpResponse del(const std::string& path, const std::unordered_map<std::string, std::string>& params = {}) = 0;

protected:
    HttpTransport() = default;
};

}  // namespace bintrade::rest::detail

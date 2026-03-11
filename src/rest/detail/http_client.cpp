#include "http_client.hpp"

#include <bintrade/core/error.hpp>

#include <algorithm>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace bintrade::rest::detail {

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using Tcp = asio::ip::tcp;

// ---------------------------------------------------------------------------
// URL parsing helper
// ---------------------------------------------------------------------------
struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string port;

    static ParsedUrl from(const std::string& url) {
        ParsedUrl result;
        auto pos = url.find("://");
        if (pos == std::string::npos) {
            throw std::invalid_argument("Invalid base_url: " + url);
        }
        result.scheme = url.substr(0, pos);

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
            result.port = (result.scheme == "https") ? "443" : "80";
        }
        return result;
    }
};

// ---------------------------------------------------------------------------
// Query-string builder
// ---------------------------------------------------------------------------
namespace {

std::string build_query_string(const std::unordered_map<std::string, std::string>& params) {
    if (params.empty()) {
        return {};
    }
    std::string query;
    for (const auto& [key, value] : params) {
        if (!query.empty()) {
            query += '&';
        }
        query += key;
        query += '=';
        query += value;
    }
    return query;
}

}  // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct HttpClient::Impl {
    RestConfig config;
    ParsedUrl parsed_url;
    std::string api_key;

    explicit Impl(RestConfig cfg) : config(std::move(cfg)), parsed_url(ParsedUrl::from(config.base_url)) {}

    [[nodiscard]] HttpResponse execute(http::verb method, const std::string& target, const std::string& body,
                                       const std::unordered_map<std::string, std::string>& extra_headers) const {
        asio::io_context ioc;
        ssl::context ctx(ssl::context::tlsv12_client);
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(config.verify_ssl ? ssl::verify_peer : ssl::verify_none);

        // Use beast::ssl_stream<beast::tcp_stream> for proper Beast integration
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // SNI hostname
        if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed_url.host.c_str())) {  // NOLINT
            throw NetworkException("Failed to set SNI hostname");
        }

        // Resolve and connect
        Tcp::resolver resolver(ioc);
        const auto results = resolver.resolve(parsed_url.host, parsed_url.port);
        beast::get_lowest_layer(stream).expires_after(config.timeout);
        beast::get_lowest_layer(stream).connect(results);

        // Disable Nagle's algorithm for lower round-trip latency.
        beast::get_lowest_layer(stream).socket().set_option(Tcp::no_delay(true));

        // SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Build HTTP request
        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, parsed_url.host);
        req.set(http::field::user_agent, "bintrade/0.1.0");
        req.set(http::field::accept, "application/json");

        if (!api_key.empty()) {
            req.set("X-MBX-APIKEY", api_key);
        }

        for (const auto& [key, value] : extra_headers) {
            req.set(key, value);
        }

        if (!body.empty()) {
            req.set(http::field::content_type, "application/x-www-form-urlencoded");
            req.body() = body;
            req.prepare_payload();
        }

        // Send request
        beast::get_lowest_layer(stream).expires_after(config.timeout);
        http::write(stream, req);

        // Receive response
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        // Build response object. Header names are lowercased for case-insensitive
        // lookup by the rate-limit tracker and any other header consumers.
        HttpResponse response;
        response.status_code = static_cast<int>(res.result_int());
        response.body = std::move(res.body());
        for (const auto& field : res) {
            auto name = std::string(field.name_string());
            std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            response.headers[std::move(name)] = std::string(field.value());
        }

        // Graceful SSL shutdown (ignore errors — server may close first)
        beast::error_code ec;
        // NOLINTNEXTLINE(bugprone-unused-return-value,cert-err33-c)
        stream.shutdown(ec);

        return response;
    }
};

// ---------------------------------------------------------------------------
// HttpClient public API
// ---------------------------------------------------------------------------
HttpClient::HttpClient(RestConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}
HttpClient::~HttpClient() = default;

void HttpClient::set_api_key(std::string api_key) {
    impl_->api_key = std::move(api_key);
}

HttpResponse HttpClient::get(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    auto query = build_query_string(params);
    auto target = path;
    if (!query.empty()) {
        target += '?';
        target += query;
    }
    return impl_->execute(http::verb::get, target, {}, {});
}

HttpResponse HttpClient::post(const std::string& path, const std::string& body, const std::unordered_map<std::string, std::string>& headers) {
    return impl_->execute(http::verb::post, path, body, headers);
}

HttpResponse HttpClient::del(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    auto query = build_query_string(params);
    auto target = path;
    if (!query.empty()) {
        target += '?';
        target += query;
    }
    return impl_->execute(http::verb::delete_, target, {}, {});
}

}  // namespace bintrade::rest::detail

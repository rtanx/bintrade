#pragma once

#include "rest/detail/http_transport.hpp"

#include <gmock/gmock.h>

#include <string>
#include <unordered_map>

namespace bintrade::test {

/// GMock mock of the HttpTransport interface.
///
/// Inject via the protected Client(unique_ptr<HttpTransport>) constructor to
/// test REST client logic without network I/O.
class MockHttpTransport : public rest::detail::HttpTransport {
    // Typedef avoids commas inside MOCK_METHOD macro arguments.
    using Params = std::unordered_map<std::string, std::string>;
    using Response = rest::detail::HttpResponse;

public:
    MockHttpTransport() = default;

    MOCK_METHOD(void, set_api_key, (std::string api_key), (override));
    MOCK_METHOD(Response, get, (const std::string& path, const Params& params), (override));
    MOCK_METHOD(Response, post, (const std::string& path, const std::string& body, const Params& headers), (override));
    MOCK_METHOD(Response, del, (const std::string& path, const Params& params), (override));
};

}  // namespace bintrade::test

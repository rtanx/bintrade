find_package(GTest CONFIG REQUIRED)

add_executable(bintrade_tests
    test_main.cpp
    # core
    unit/core/types_test.cpp
    unit/core/error_test.cpp
    # auth
    unit/auth/signer_test.cpp
    unit/auth/credentials_test.cpp
    # REST detail parsers (include private src/ headers)
    unit/rest/detail/json_parse_test.cpp
    # REST client tests (use MockHttpTransport)
    unit/rest/client_test.cpp
    unit/rest/market_data_client_test.cpp
    unit/rest/trading_client_test.cpp
    unit/rest/account_client_test.cpp
    unit/rest/async_client_test.cpp
    # WebSocket detail parsers (include private src/ headers)
    unit/ws/detail/ws_parse_test.cpp
    # WebSocket UserStream tests
    unit/ws/user_stream_test.cpp
    # WebSocket MarketStream dispatch tests
    unit/ws/market_stream_test.cpp
)

target_include_directories(bintrade_tests
    PRIVATE
        # Expose private detail headers (json_parse.hpp, ws_parse.hpp) to tests.
        # These are internal implementation details tested directly in Phase 1.
        ${PROJECT_SOURCE_DIR}/src
        # Mock and fixture headers for Phase 2+ REST client tests.
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
)

target_link_libraries(bintrade_tests
    PRIVATE
        bintrade::bintrade
        GTest::gtest
        GTest::gmock
)

enable_coverage(bintrade_tests)

include(GoogleTest)
gtest_discover_tests(bintrade_tests)

# Integration tests (separate target, not run by default)
add_executable(bintrade_integration_tests
    test_main.cpp
    integration/rest_integration_test.cpp
)

target_link_libraries(bintrade_integration_tests
    PRIVATE
        bintrade::bintrade
        GTest::gtest
        GTest::gmock
)

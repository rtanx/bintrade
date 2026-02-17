find_package(GTest CONFIG REQUIRED)

add_executable(bintrade_tests
    test_main.cpp
    unit/core/types_test.cpp
    unit/auth/signer_test.cpp
)

target_link_libraries(bintrade_tests
    PRIVATE
        bintrade::bintrade
        GTest::gtest
        GTest::gmock
)

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

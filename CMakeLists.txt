cmake_minimum_required(VERSION 3.21)

project(bintrade
    VERSION 0.1.0
    DESCRIPTION "Binance Trading API Wrapper Library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(StandardProjectSettings)
include(CompilerWarnings)
include(Sanitizers)
include(Coverage)

# Build options
option(BINTRADE_BUILD_TESTS "Build tests" ON)
option(BINTRADE_BUILD_EXAMPLES "Build examples" ON)
option(BINTRADE_BUILD_SHARED "Build shared library" OFF)
option(BINTRADE_BUILD_BENCHMARKS "Build benchmarks (use Release builds for meaningful results)" OFF)

# Ensure imported targets use -isystem (suppresses warnings in third-party headers)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED OFF)

# Dependencies (via vcpkg)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Generate version header
configure_file(
    "${PROJECT_SOURCE_DIR}/include/bintrade/version.hpp.in"
    "${PROJECT_BINARY_DIR}/include/bintrade/version.hpp"
    @ONLY
)

# Library sources
set(BINTRADE_SOURCES
    src/core/error.cpp
    src/core/types.cpp
    src/core/config.cpp
    src/auth/credentials.cpp
    src/auth/signer.cpp
    src/models/common.cpp
    src/models/order.cpp
    src/models/trade.cpp
    src/models/market_data.cpp
    src/models/account.cpp
    src/rest/detail/http_client.cpp
    src/rest/client.cpp
    src/rest/market_data_client.cpp
    src/rest/trading_client.cpp
    src/rest/account_client.cpp
    src/ws/detail/websocket_client.cpp
    src/ws/client.cpp
    src/ws/market_stream.cpp
    src/ws/user_stream.cpp
)

# Library target
if(BINTRADE_BUILD_SHARED)
    add_library(bintrade SHARED ${BINTRADE_SOURCES})
else()
    add_library(bintrade STATIC ${BINTRADE_SOURCES})
endif()

add_library(bintrade::bintrade ALIAS bintrade)

target_include_directories(bintrade
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(bintrade
    PUBLIC
        nlohmann_json::nlohmann_json
    PRIVATE
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        fmt::fmt
)

set_project_warnings(bintrade)
enable_sanitizers(bintrade)
enable_coverage(bintrade)

# Tests
if(BINTRADE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BINTRADE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Benchmarks
if(BINTRADE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS bintrade
    EXPORT bintradeTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/bintrade
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES "${PROJECT_BINARY_DIR}/include/bintrade/version.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bintrade
)

install(EXPORT bintradeTargets
    FILE bintradeTargets.cmake
    NAMESPACE bintrade::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bintrade
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/bintradeConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/bintradeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/bintradeConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/bintradeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/bintradeConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bintrade
)

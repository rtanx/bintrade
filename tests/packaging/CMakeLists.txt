# Downstream consumer smoke test for find_package(bintrade).
#
# This is a *standalone* CMake project (note the top-level project() call)
# that is intentionally NOT part of the main bintrade build.  It is driven
# by `make packaging-test`, which:
#
#   1. Installs bintrade to a temporary prefix.
#   2. Configures this project with -Dbintrade_DIR=<prefix>/lib/cmake/bintrade.
#   3. Builds the consumer binary and runs it.
#
# A green run proves that the installed bintradeConfig.cmake and its
# transitive find_dependency() calls are wired correctly.

cmake_minimum_required(VERSION 3.21)
project(bintrade_consumer_smoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(bintrade CONFIG REQUIRED)

add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE bintrade::bintrade)

find_package(benchmark CONFIG REQUIRED)

add_executable(bintrade_benchmarks
    spsc_queue_bench.cpp
    json_parse_bench.cpp
    callback_dispatch_bench.cpp
)

target_include_directories(bintrade_benchmarks
    PRIVATE
        # Access private src/ headers (ws_parse.hpp forwarder, json_parse.hpp).
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(bintrade_benchmarks
    PRIVATE
        bintrade::bintrade
        benchmark::benchmark
        benchmark::benchmark_main
)

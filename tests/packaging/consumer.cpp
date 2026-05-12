// Downstream smoke test: must compile, link, and run against the installed
// bintrade package (find_package(bintrade CONFIG REQUIRED)).  Touches the
// umbrella header, the version constants, the logger, and a few public
// types so the linker has to resolve transitive PUBLIC dependencies
// (spdlog, fmt, nlohmann_json).

#include <bintrade/bintrade.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

int main() {
    // Compile-time sanity: the version_major macro is wired through.
    static_assert(bintrade::version_major >= 0, "version_major present");

    std::cout << "bintrade " << bintrade::version << " (" << bintrade::version_major << "." << bintrade::version_minor << "."
              << bintrade::version_patch << ")\n";

    // Exercise the logger -- forces the spdlog/fmt linkage to resolve.
    bintrade::LogConfig cfg;
    cfg.use_async = false;
    cfg.level = bintrade::LogLevel::Info;
    bintrade::init_logging(cfg);
    bintrade::logger()->info("downstream consumer up");

    // Touch a few public types that pull in PUBLIC interface libraries.
    bintrade::RestConfig rest;
    bintrade::WebSocketConfig ws;
    static_cast<void>(rest);
    static_cast<void>(ws);

    return EXIT_SUCCESS;
}

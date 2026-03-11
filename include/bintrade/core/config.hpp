#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace bintrade {

struct RestConfig {
    std::string base_url = "https://api.binance.com";
    std::chrono::seconds timeout = std::chrono::seconds(30);
    bool use_testnet = false;
    std::optional<std::string> proxy;
    bool verify_ssl = true;
};

struct WebSocketConfig {
    std::string base_url = "wss://stream.binance.com:9443";
    bool use_testnet = false;
    std::chrono::seconds ping_interval = std::chrono::seconds(30);
    std::chrono::seconds reconnect_interval = std::chrono::seconds(5);
    int max_reconnect_attempts = 10;
    // CPU core to pin the I/O thread to (-1 = no pinning).
    // Effective on Linux only; silently ignored on other platforms.
    int io_core_id = -1;
};

}  // namespace bintrade

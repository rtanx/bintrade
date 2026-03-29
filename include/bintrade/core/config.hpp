#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace bintrade {

// Controls how WebSocket messages are dispatched from the I/O thread
// to user callbacks.
enum class DispatchMode {
    Inline,  // Parse JSON and invoke callbacks directly on the I/O thread (default).
    Queued   // Enqueue raw messages to an SPSC ring buffer; a consumer thread parses and dispatches.
};

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
    // Linux  : hard pinning via pthread_setaffinity_np.
    // macOS  : advisory scheduling hint via THREAD_AFFINITY_POLICY; the
    //          kernel may still schedule the thread on a different core.
    // Windows: SetThreadAffinityMask; limited to cores 0-63.
    int io_core_id = -1;
};

}  // namespace bintrade

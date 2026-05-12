# bintrade

[![CI](https://github.com/rtanx/bintrade/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/rtanx/bintrade/actions/workflows/ci.yml)

A modern C++20 wrapper around the Binance Spot REST and WebSocket APIs,
written with HFT-oriented engineering practices: persistent connections,
zero-copy hot paths, lock-free SPSC handoff between I/O and strategy
threads, and structured async logging that never blocks the WebSocket
reader.

This is a library, not a strategy framework. It gives you typed
request/response models and live streams; the trading logic on top is up
to you.

---

## Features

- **Async REST transport**. One persistent `boost::asio::io_context` per
  client, connection pool to `api.binance.com`, SSL session reuse, and
  every endpoint exposed as both a synchronous call and a C++20
  coroutine (`co_await`).
- **WebSocket streams**. Persistent connections, automatic reconnect
  with exponential backoff + jitter, configurable ping interval, and
  hot-path CPU pinning via `WebSocketConfig::io_core_id`.
- **Compile-time callback dispatch**. `TypedMarketStream<Handler>` and
  `TypedUserStream<Handler>` use CRTP to avoid `std::function`
  indirection on the hot path. Convenience `std::function` overloads are
  still provided.
- **SPSC ring buffer**. `bintrade::SpscQueue<T, Capacity>` is a header
  only, cache-line padded, power-of-two bounded queue used internally
  for the WebSocket I/O -> strategy handoff when `DispatchMode::Queued`
  is selected.
- **Structured logging**. `bintrade::logger()` returns a `spdlog::logger`
  configured with an asynchronous sink by default, so log calls never
  block the WS reader. Sync mode is available for deterministic tests.
- **Cross-platform**. CI builds and tests on Ubuntu, macOS, and Windows.
  Thread pinning is abstracted in
  [src/core/platform/thread_affinity.hpp](src/core/platform/thread_affinity.hpp).
- **Sanitizer-clean**. A dedicated CI job runs the full unit-test suite
  under AddressSanitizer + UndefinedBehaviorSanitizer.

---

## Requirements

- **Compilers**: clang >= 15, gcc >= 11, or MSVC 2022 (Visual Studio
  17.5+). C++20 mode is required (`-std=c++20` / `/std:c++20`).
- **CMake**: 3.21 or newer.
- **vcpkg**: required for dependency management. Set
  `VCPKG_ROOT` in your shell before configuring. Manifest mode is used,
  so dependencies are listed in [vcpkg.json](vcpkg.json) and resolved
  automatically.
- **OpenSSL**: 1.1.1 or 3.x (provided via vcpkg).
- **Doxygen** (optional): only needed for `make docs`.

Runtime dependencies (linked PUBLIC, visible to your code):

- [nlohmann/json](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog)
- [fmt](https://github.com/fmtlib/fmt)

Boost.Beast / Boost.Asio / Boost.System and OpenSSL are linked PRIVATE.

---

## Building

All developer tasks are driven through the `Makefile` wrapper. Never
invoke CMake or `clang-tidy` directly; the Makefile keeps the build
directory layout consistent.

```bash
export VCPKG_ROOT=/path/to/vcpkg

make build-debug      # Debug build with sanitizers enabled in CI
make build-release    # Optimised release build (-O3 -march=native)
make test             # Run unit tests via CTest
make bench            # Build and run benchmarks (Release)
make lint             # Run clang-tidy across include/ and src/
make format           # clang-format all sources
make docs             # Generate Doxygen HTML into build/docs/html/
make install          # CMake install into CMAKE_INSTALL_PREFIX
```

CMake presets cover all supported platforms; the Makefile selects them
for you, but you can configure them by hand too:

```bash
cmake --preset linux-debug          # Linux x86-64, debug
cmake --preset linux-release        # Linux x86-64, release
cmake --preset linux-asan           # Linux + ASan + UBSan
cmake --preset macos-arm64-debug    # Apple Silicon, debug
cmake --preset macos-x64-release    # Intel macOS, release
cmake --preset windows-debug        # MSVC x64-windows-static
cmake --preset windows-release      # MSVC x64-windows-static
```

---

## Quick start

### Synchronous REST

```cpp
#include <bintrade/bintrade.hpp>
#include <iostream>

int main() {
    bintrade::rest::MarketDataClient client;

    auto ticker = client.get_price_ticker("BTCUSDT");
    std::cout << "BTC/USDT price: $" << ticker.price << "\n";

    auto book = client.get_order_book("BTCUSDT", 5);
    std::cout << book.bids.size() << " bids, " << book.asks.size() << " asks\n";
}
```

### Asynchronous REST (`co_await`)

```cpp
#include <bintrade/bintrade.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <iostream>

namespace asio = boost::asio;

asio::awaitable<void> fetch(bintrade::rest::MarketDataClient& client) {
    auto ticker_fut = asio::co_spawn(co_await asio::this_coro::executor,
                                     client.async_get_price_ticker("BTCUSDT"),
                                     asio::use_future);
    auto book_fut = asio::co_spawn(co_await asio::this_coro::executor,
                                   client.async_get_order_book("BTCUSDT", 5),
                                   asio::use_future);

    auto ticker = ticker_fut.get();
    auto book = book_fut.get();
    std::cout << ticker.price << "  " << book.bids.size() << "/" << book.asks.size() << "\n";
}

int main() {
    bintrade::rest::MarketDataClient client;
    asio::io_context ioc;
    auto fut = asio::co_spawn(ioc, fetch(client), asio::use_future);
    ioc.run();
    fut.get();
}
```

### WebSocket market stream

```cpp
#include <bintrade/bintrade.hpp>
#include <iostream>
#include <thread>

int main() {
    bintrade::ws::MarketStream stream;

    stream.subscribe_trades("BTCUSDT", [](const bintrade::models::Trade& t) {
        std::cout << "trade: " << t.price << " x " << t.qty
                  << (t.is_buyer ? " BUY" : " SELL") << "\n";
    });

    std::this_thread::sleep_for(std::chrono::seconds(60));
}
```

### CRTP typed stream (zero-overhead dispatch)

```cpp
struct MyHandler {
    void on_trade(const bintrade::models::Trade& t) noexcept {
        // ... compiled into a direct call, no std::function indirection.
    }
};

MyHandler h;
bintrade::ws::TypedMarketStream<MyHandler> stream(h);
stream.subscribe_trades("BTCUSDT");
```

More examples live in [examples/](examples/).

---

## Configuration

### `bintrade::RestConfig`

| Field         | Default                       | Notes                                  |
| ------------- | ----------------------------- | -------------------------------------- |
| `base_url`    | `https://api.binance.com`     | Override for testnet / proxies         |
| `timeout`     | 30 s                          | Per-request timeout                    |
| `use_testnet` | false                         | Convenience flag                       |
| `proxy`       | nullopt                       | Optional outbound proxy                |
| `verify_ssl`  | true                          | Disable for self-signed local servers  |

### `bintrade::WebSocketConfig`

| Field                  | Default                              | Notes                                   |
| ---------------------- | ------------------------------------ | --------------------------------------- |
| `base_url`             | `wss://stream.binance.com:9443`      |                                         |
| `use_testnet`          | false                                |                                         |
| `ping_interval`        | 30 s                                 | Keep-alive ping cadence                 |
| `reconnect_interval`   | 5 s                                  | First-attempt backoff base              |
| `max_reconnect_attempts` | 10                                 | Hard cap on reconnect retries           |
| `io_core_id`           | -1                                   | CPU core for I/O thread (-1 = no pin)   |

### `bintrade::LogConfig`

| Field                | Default                                          | Notes                                   |
| -------------------- | ------------------------------------------------ | --------------------------------------- |
| `level`              | `Info`                                           | Trace / Debug / Info / Warn / Error / Critical / Off |
| `pattern`            | `[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [t:%t] %v`     | spdlog format string                    |
| `use_async`          | true                                             | Async sink with overrun_oldest policy   |
| `async_queue_size`   | 8192                                             | Power of two recommended                |
| `async_thread_count` | 1                                                | spdlog worker threads                   |

Call `bintrade::init_logging(cfg)` before the first call to
`bintrade::logger()` to override defaults. `bintrade::set_log_sink(...)`
plugs in your own `spdlog::sink_ptr`.

---

## Using the library from CMake

After `make install`, downstream projects can pull `bintrade` in via
`find_package`:

```cmake
find_package(bintrade CONFIG REQUIRED)

add_executable(my_bot main.cpp)
target_link_libraries(my_bot PRIVATE bintrade::bintrade)
```

A pkg-config file is also installed at
`<prefix>/lib/pkgconfig/bintrade.pc` for non-CMake build systems:

```bash
pkg-config --cflags --libs bintrade
```

### Consuming via vcpkg overlay

This repository ships a vcpkg port overlay under
[ports/bintrade/](ports/bintrade/). To consume it from another vcpkg
manifest project:

```bash
vcpkg install bintrade --overlay-ports=/path/to/bintrade/ports
```

---

## Project layout

```
include/bintrade/   Public headers (only entries under detail/ are private).
src/                Library implementation, including detail/ subfolders.
tests/              Unit tests; integration tests are opt-in (see below).
benchmarks/         Google Benchmark micro-benchmarks.
examples/           Standalone executables exercising the public API.
cmake/              CMake helpers, Config.cmake template, .pc.in template.
ports/bintrade/     vcpkg overlay port for downstream installs.
.github/workflows/  CI definitions (build + test + asan + lint).
```

---

## Integration tests

Integration tests live in [tests/integration/](tests/integration/) and
talk to the Binance testnet (`https://testnet.binance.vision` and
`wss://testnet.binance.vision`). They are **off by default** because
they require live network access. Enable them explicitly:

```bash
make integration-test
```

This configures CMake with `-DBINTRADE_ENABLE_INTEGRATION_TESTS=ON`,
builds the `bintrade_integration_tests` binary, and runs it.

Signed-endpoint tests are skipped automatically unless
`BINTRADE_TEST_API_KEY` and `BINTRADE_TEST_API_SECRET` are present in
the environment.

---

## Development workflow

1. `make format` -- run clang-format across all changes.
2. `make build-debug` -- compile with warnings as errors.
3. `make lint` -- clang-tidy must stay clean (no new suppressions).
4. `make test` -- the full unit-test suite must pass.

CI runs the same four steps on every push, plus the ASan + UBSan job.

See [AGENTS.md](AGENTS.md) for the engineering principles the project
adheres to (low-latency hot-path rules, ASCII-only sources, no
`std::mutex` on hot paths, no virtual dispatch on the WS reader, etc.).

---

## License

MIT. See the package metadata in [vcpkg.json](vcpkg.json).

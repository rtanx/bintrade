// Demonstrates the async REST API introduced in Sprint 2 Phase 1.
//
// Fetches a price ticker and order book concurrently using C++20 coroutines,
// then prints the results.  Requires a live network connection to api.binance.com.
//
// Build:
//   make build-debug    (or build-release)
//
// Run:
//   ./build/debug/examples/async_market_data_example

#include <bintrade/bintrade.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <iostream>

namespace asio = boost::asio;

namespace {
// ---------------------------------------------------------------------------
// A coroutine that fetches several market-data endpoints concurrently and
// prints the results.
// ---------------------------------------------------------------------------

asio::awaitable<void> fetch_market_data(bintrade::rest::MarketDataClient* client) {
    // Kick off both requests without waiting for either to complete first.
    // co_spawn(..., use_future) returns a std::future<T>; the underlying
    // HttpClient serialises I/O on its own thread, so the two coroutines
    // make independent HTTP requests and resume here once both respond.

    auto ticker_fut = asio::co_spawn(co_await asio::this_coro::executor, client->async_get_price_ticker("BTCUSDT"), asio::use_future);

    auto book_fut = asio::co_spawn(co_await asio::this_coro::executor, client->async_get_order_book("BTCUSDT", 5), asio::use_future);

    // Await both futures (they resolve as soon as HttpClient posts results).
    auto ticker = ticker_fut.get();
    auto book = book_fut.get();

    std::cout << "BTC/USDT async price : $" << ticker.price << "\n";
    std::cout << "Order book (top 5)   : " << book.bids.size() << " bids, " << book.asks.size() << " asks\n";
    if (!book.bids.empty()) {
        std::cout << "  Best bid: $" << book.bids[0].price << " x " << book.bids[0].quantity << "\n";
    }
    if (!book.asks.empty()) {
        std::cout << "  Best ask: $" << book.asks[0].price << " x " << book.asks[0].quantity << "\n";
    }

    // Also demonstrate async_get_ticker_24h sequentially.
    auto ticker24h = co_await client->async_get_ticker_24h("BTCUSDT");
    std::cout << "24h high: $" << ticker24h.high << "  low: $" << ticker24h.low << "  volume: " << ticker24h.volume << "\n";
}
}  // namespace

int main() {
    try {
        bintrade::rest::MarketDataClient client;

        asio::io_context ioc;
        auto fut = asio::co_spawn(ioc, fetch_market_data(&client), asio::use_future);
        ioc.run();
        fut.get();  // re-throw any exception from the coroutine

    } catch (const bintrade::ApiException& e) {
        std::cerr << "API error [" << e.code() << "]: " << e.what() << "\n";
        return 1;
    } catch (const bintrade::Exception& e) {
        std::cerr << "Bintrade error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

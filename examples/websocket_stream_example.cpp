#include <bintrade/bintrade.hpp>

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    try {
        bintrade::ws::MarketStream stream;

        stream.subscribe_trades("BTCUSDT", [](const bintrade::models::Trade& trade) {
            std::cout << "Trade: price=" << trade.price << " qty=" << trade.qty
                      << (trade.is_buyer ? " [BUY]" : " [SELL]") << "\n";
        });

        std::cout << "Streaming BTC/USDT trades... Press Ctrl+C to stop\n";
        std::this_thread::sleep_for(std::chrono::seconds(60));

    } catch (const bintrade::Exception& e) {
        std::cerr << "Bintrade error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

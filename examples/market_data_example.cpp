#include <bintrade/bintrade.hpp>

#include <iostream>

int main() {
    try {
        bintrade::rest::MarketDataClient client;

        auto ticker = client.get_price_ticker("BTCUSDT");
        std::cout << "BTC/USDT Price: $" << ticker.price << "\n";

        auto orderbook = client.get_order_book("BTCUSDT", 5);
        std::cout << "Order book entries: " << orderbook.bids.size() << " bids, "
                  << orderbook.asks.size() << " asks\n";

    } catch (const bintrade::Exception& e) {
        std::cerr << "Bintrade error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

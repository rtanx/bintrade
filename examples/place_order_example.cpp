#include <bintrade/bintrade.hpp>

#include <iostream>

int main() {
    try {
        bintrade::Credentials creds("YOUR_API_KEY", "YOUR_API_SECRET");

        bintrade::RestConfig config;
        config.use_testnet = true;
        config.base_url = "https://testnet.binance.vision";

        bintrade::rest::TradingClient client(config, std::move(creds));

        bintrade::models::NewOrderRequest order;
        order.symbol = "BTCUSDT";
        order.side = bintrade::Side::Buy;
        order.type = bintrade::OrderType::Limit;
        order.time_in_force = bintrade::TimeInForce::GTC;
        order.quantity = 0.001;
        order.price = 40000.0;

        auto result = client.new_order(order);
        std::cout << "Order placed! ID: " << result.order_id << "\n";
        std::cout << "Status: " << bintrade::to_string(result.status) << "\n";

    } catch (const bintrade::Exception& e) {
        std::cerr << "Bintrade error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

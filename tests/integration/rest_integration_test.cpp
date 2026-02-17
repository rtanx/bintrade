#include <bintrade/rest/market_data_client.hpp>

#include <gtest/gtest.h>

namespace bintrade::test {

// Integration tests require network access and are not run by default.
// Run with: ./bintrade_integration_tests

class RestIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use testnet by default for integration tests
        RestConfig config;
        config.use_testnet = true;
        config.base_url = "https://testnet.binance.vision";
        // client_ = std::make_unique<rest::MarketDataClient>(config);
    }

    // std::unique_ptr<rest::MarketDataClient> client_;
};

TEST_F(RestIntegrationTest, DISABLED_PingServer) {
    // TODO: Enable when REST client is implemented
    // EXPECT_TRUE(client_->ping());
}

TEST_F(RestIntegrationTest, DISABLED_GetServerTime) {
    // TODO: Enable when REST client is implemented
    // auto time = client_->server_time();
    // auto now = std::chrono::system_clock::now();
    // auto diff = std::chrono::abs(now - time);
    // EXPECT_LT(diff, std::chrono::seconds(5));
}

}  // namespace bintrade::test

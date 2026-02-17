#include <bintrade/core/types.hpp>

#include <gtest/gtest.h>

namespace bintrade::test {

TEST(TypesTest, SideToString) {
    EXPECT_EQ(to_string(Side::Buy), "BUY");
    EXPECT_EQ(to_string(Side::Sell), "SELL");
}

TEST(TypesTest, SideFromString) {
    EXPECT_EQ(side_from_string("BUY"), Side::Buy);
    EXPECT_EQ(side_from_string("SELL"), Side::Sell);
    EXPECT_THROW(side_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, OrderTypeToString) {
    EXPECT_EQ(to_string(OrderType::Limit), "LIMIT");
    EXPECT_EQ(to_string(OrderType::Market), "MARKET");
    EXPECT_EQ(to_string(OrderType::StopLoss), "STOP_LOSS");
    EXPECT_EQ(to_string(OrderType::StopLossLimit), "STOP_LOSS_LIMIT");
    EXPECT_EQ(to_string(OrderType::TakeProfit), "TAKE_PROFIT");
    EXPECT_EQ(to_string(OrderType::TakeProfitLimit), "TAKE_PROFIT_LIMIT");
    EXPECT_EQ(to_string(OrderType::LimitMaker), "LIMIT_MAKER");
}

TEST(TypesTest, OrderTypeFromString) {
    EXPECT_EQ(order_type_from_string("LIMIT"), OrderType::Limit);
    EXPECT_EQ(order_type_from_string("MARKET"), OrderType::Market);
    EXPECT_THROW(order_type_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, TimeInForceRoundTrip) {
    EXPECT_EQ(time_in_force_from_string("GTC"), TimeInForce::GTC);
    EXPECT_EQ(time_in_force_from_string("IOC"), TimeInForce::IOC);
    EXPECT_EQ(time_in_force_from_string("FOK"), TimeInForce::FOK);
}

TEST(TypesTest, OrderStatusRoundTrip) {
    EXPECT_EQ(order_status_from_string("NEW"), OrderStatus::New);
    EXPECT_EQ(order_status_from_string("FILLED"), OrderStatus::Filled);
    EXPECT_EQ(order_status_from_string("CANCELED"), OrderStatus::Canceled);
    EXPECT_EQ(to_string(OrderStatus::PartiallyFilled), "PARTIALLY_FILLED");
}

}  // namespace bintrade::test

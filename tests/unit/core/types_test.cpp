#include <bintrade/core/types.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

namespace bintrade::test {

// ---------------------------------------------------------------------------
// Side -- exhaustive round-trips
// ---------------------------------------------------------------------------

TEST(TypesTest, SideToString) {
    EXPECT_EQ(to_string(Side::Buy), "BUY");
    EXPECT_EQ(to_string(Side::Sell), "SELL");
}

TEST(TypesTest, SideFromString) {
    EXPECT_EQ(side_from_string("BUY"), Side::Buy);
    EXPECT_EQ(side_from_string("SELL"), Side::Sell);
}

TEST(TypesTest, SideFromStringThrowsOnInvalid) {
    EXPECT_THROW(static_cast<void>(side_from_string("INVALID")), std::invalid_argument);
}

TEST(TypesTest, SideFromStringThrowsOnEmpty) {
    EXPECT_THROW(static_cast<void>(side_from_string("")), std::invalid_argument);
}

TEST(TypesTest, SideFromStringThrowsOnMixedCase) {
    EXPECT_THROW(static_cast<void>(side_from_string("Buy")), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(side_from_string("sell")), std::invalid_argument);
}

TEST(TypesTest, SideRoundTrip) {
    EXPECT_EQ(side_from_string(to_string(Side::Buy)), Side::Buy);
    EXPECT_EQ(side_from_string(to_string(Side::Sell)), Side::Sell);
}

// ---------------------------------------------------------------------------
// OrderType -- all 7 values
// ---------------------------------------------------------------------------

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
    EXPECT_EQ(order_type_from_string("STOP_LOSS"), OrderType::StopLoss);
    EXPECT_EQ(order_type_from_string("STOP_LOSS_LIMIT"), OrderType::StopLossLimit);
    EXPECT_EQ(order_type_from_string("TAKE_PROFIT"), OrderType::TakeProfit);
    EXPECT_EQ(order_type_from_string("TAKE_PROFIT_LIMIT"), OrderType::TakeProfitLimit);
    EXPECT_EQ(order_type_from_string("LIMIT_MAKER"), OrderType::LimitMaker);
}

TEST(TypesTest, OrderTypeFromStringThrowsOnInvalid) {
    EXPECT_THROW(static_cast<void>(order_type_from_string("INVALID")), std::invalid_argument);
}

TEST(TypesTest, OrderTypeFromStringThrowsOnEmpty) {
    EXPECT_THROW(static_cast<void>(order_type_from_string("")), std::invalid_argument);
}

TEST(TypesTest, OrderTypeFromStringThrowsOnMixedCase) {
    EXPECT_THROW(static_cast<void>(order_type_from_string("limit")), std::invalid_argument);
}

TEST(TypesTest, OrderTypeRoundTrip) {
    EXPECT_EQ(order_type_from_string(to_string(OrderType::Limit)), OrderType::Limit);
    EXPECT_EQ(order_type_from_string(to_string(OrderType::Market)), OrderType::Market);
    EXPECT_EQ(order_type_from_string(to_string(OrderType::StopLoss)), OrderType::StopLoss);
    EXPECT_EQ(order_type_from_string(to_string(OrderType::StopLossLimit)), OrderType::StopLossLimit);
    EXPECT_EQ(order_type_from_string(to_string(OrderType::TakeProfit)), OrderType::TakeProfit);
    EXPECT_EQ(order_type_from_string(to_string(OrderType::TakeProfitLimit)), OrderType::TakeProfitLimit);
    EXPECT_EQ(order_type_from_string(to_string(OrderType::LimitMaker)), OrderType::LimitMaker);
}

// ---------------------------------------------------------------------------
// TimeInForce -- all 3 values
// ---------------------------------------------------------------------------

TEST(TypesTest, TimeInForceToString) {
    EXPECT_EQ(to_string(TimeInForce::GTC), "GTC");
    EXPECT_EQ(to_string(TimeInForce::IOC), "IOC");
    EXPECT_EQ(to_string(TimeInForce::FOK), "FOK");
}

TEST(TypesTest, TimeInForceFromString) {
    EXPECT_EQ(time_in_force_from_string("GTC"), TimeInForce::GTC);
    EXPECT_EQ(time_in_force_from_string("IOC"), TimeInForce::IOC);
    EXPECT_EQ(time_in_force_from_string("FOK"), TimeInForce::FOK);
}

TEST(TypesTest, TimeInForceFromStringThrowsOnInvalid) {
    EXPECT_THROW(static_cast<void>(time_in_force_from_string("INVALID")), std::invalid_argument);
}

TEST(TypesTest, TimeInForceFromStringThrowsOnEmpty) {
    EXPECT_THROW(static_cast<void>(time_in_force_from_string("")), std::invalid_argument);
}

TEST(TypesTest, TimeInForceRoundTrip) {
    EXPECT_EQ(time_in_force_from_string(to_string(TimeInForce::GTC)), TimeInForce::GTC);
    EXPECT_EQ(time_in_force_from_string(to_string(TimeInForce::IOC)), TimeInForce::IOC);
    EXPECT_EQ(time_in_force_from_string(to_string(TimeInForce::FOK)), TimeInForce::FOK);
}

// ---------------------------------------------------------------------------
// OrderStatus -- all 7 values
// ---------------------------------------------------------------------------

TEST(TypesTest, OrderStatusToString) {
    EXPECT_EQ(to_string(OrderStatus::New), "NEW");
    EXPECT_EQ(to_string(OrderStatus::PartiallyFilled), "PARTIALLY_FILLED");
    EXPECT_EQ(to_string(OrderStatus::Filled), "FILLED");
    EXPECT_EQ(to_string(OrderStatus::Canceled), "CANCELED");
    EXPECT_EQ(to_string(OrderStatus::PendingCancel), "PENDING_CANCEL");
    EXPECT_EQ(to_string(OrderStatus::Rejected), "REJECTED");
    EXPECT_EQ(to_string(OrderStatus::Expired), "EXPIRED");
}

TEST(TypesTest, OrderStatusFromString) {
    EXPECT_EQ(order_status_from_string("NEW"), OrderStatus::New);
    EXPECT_EQ(order_status_from_string("PARTIALLY_FILLED"), OrderStatus::PartiallyFilled);
    EXPECT_EQ(order_status_from_string("FILLED"), OrderStatus::Filled);
    EXPECT_EQ(order_status_from_string("CANCELED"), OrderStatus::Canceled);
    EXPECT_EQ(order_status_from_string("PENDING_CANCEL"), OrderStatus::PendingCancel);
    EXPECT_EQ(order_status_from_string("REJECTED"), OrderStatus::Rejected);
    EXPECT_EQ(order_status_from_string("EXPIRED"), OrderStatus::Expired);
}

TEST(TypesTest, OrderStatusFromStringThrowsOnInvalid) {
    EXPECT_THROW(static_cast<void>(order_status_from_string("INVALID")), std::invalid_argument);
}

TEST(TypesTest, OrderStatusFromStringThrowsOnEmpty) {
    EXPECT_THROW(static_cast<void>(order_status_from_string("")), std::invalid_argument);
}

TEST(TypesTest, OrderStatusRoundTrip) {
    EXPECT_EQ(order_status_from_string(to_string(OrderStatus::New)), OrderStatus::New);
    EXPECT_EQ(order_status_from_string(to_string(OrderStatus::PartiallyFilled)), OrderStatus::PartiallyFilled);
    EXPECT_EQ(order_status_from_string(to_string(OrderStatus::Filled)), OrderStatus::Filled);
    EXPECT_EQ(order_status_from_string(to_string(OrderStatus::Canceled)), OrderStatus::Canceled);
    EXPECT_EQ(order_status_from_string(to_string(OrderStatus::PendingCancel)), OrderStatus::PendingCancel);
    EXPECT_EQ(order_status_from_string(to_string(OrderStatus::Rejected)), OrderStatus::Rejected);
    EXPECT_EQ(order_status_from_string(to_string(OrderStatus::Expired)), OrderStatus::Expired);
}

}  // namespace bintrade::test

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace bintrade {

using Timestamp = std::chrono::system_clock::time_point;
using Duration = std::chrono::milliseconds;
using OrderId = std::uint64_t;
using ClientOrderId = std::string;
using Symbol = std::string;
using Price = double;
using Quantity = double;

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market,
    StopLoss,
    StopLossLimit,
    TakeProfit,
    TakeProfitLimit,
    LimitMaker
};

enum class TimeInForce {
    GTC,
    IOC,
    FOK
};

enum class OrderStatus {
    New,
    PartiallyFilled,
    Filled,
    Canceled,
    PendingCancel,
    Rejected,
    Expired
};

[[nodiscard]] std::string_view to_string(Side side);
[[nodiscard]] std::string_view to_string(OrderType type);
[[nodiscard]] std::string_view to_string(TimeInForce tif);
[[nodiscard]] std::string_view to_string(OrderStatus status);

[[nodiscard]] Side side_from_string(std::string_view str);
[[nodiscard]] OrderType order_type_from_string(std::string_view str);
[[nodiscard]] TimeInForce time_in_force_from_string(std::string_view str);
[[nodiscard]] OrderStatus order_status_from_string(std::string_view str);

}  // namespace bintrade

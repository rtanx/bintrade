#include <bintrade/core/types.hpp>

#include <stdexcept>
#include <string_view>

namespace bintrade {

std::string_view to_string(Side side) {
    switch (side) {
    case Side::Buy:
        return "BUY";
    case Side::Sell:
        return "SELL";
    }
    return "UNKNOWN";
}

std::string_view to_string(OrderType type) {
    switch (type) {
    case OrderType::Limit:
        return "LIMIT";
    case OrderType::Market:
        return "MARKET";
    case OrderType::StopLoss:
        return "STOP_LOSS";
    case OrderType::StopLossLimit:
        return "STOP_LOSS_LIMIT";
    case OrderType::TakeProfit:
        return "TAKE_PROFIT";
    case OrderType::TakeProfitLimit:
        return "TAKE_PROFIT_LIMIT";
    case OrderType::LimitMaker:
        return "LIMIT_MAKER";
    }
    return "UNKNOWN";
}

std::string_view to_string(TimeInForce tif) {
    switch (tif) {
    case TimeInForce::GTC:
        return "GTC";
    case TimeInForce::IOC:
        return "IOC";
    case TimeInForce::FOK:
        return "FOK";
    }
    return "UNKNOWN";
}

std::string_view to_string(OrderStatus status) {
    switch (status) {
    case OrderStatus::New:
        return "NEW";
    case OrderStatus::PartiallyFilled:
        return "PARTIALLY_FILLED";
    case OrderStatus::Filled:
        return "FILLED";
    case OrderStatus::Canceled:
        return "CANCELED";
    case OrderStatus::PendingCancel:
        return "PENDING_CANCEL";
    case OrderStatus::Rejected:
        return "REJECTED";
    case OrderStatus::Expired:
        return "EXPIRED";
    }
    return "UNKNOWN";
}

Side side_from_string(std::string_view str) {
    if (str == "BUY") {
        return Side::Buy;
    }
    if (str == "SELL") {
        return Side::Sell;
    }
    throw std::invalid_argument(std::string("Unknown side: ") + std::string(str));
}

OrderType order_type_from_string(std::string_view str) {
    if (str == "LIMIT") {
        return OrderType::Limit;
    }
    if (str == "MARKET") {
        return OrderType::Market;
    }
    if (str == "STOP_LOSS") {
        return OrderType::StopLoss;
    }
    if (str == "STOP_LOSS_LIMIT") {
        return OrderType::StopLossLimit;
    }
    if (str == "TAKE_PROFIT") {
        return OrderType::TakeProfit;
    }
    if (str == "TAKE_PROFIT_LIMIT") {
        return OrderType::TakeProfitLimit;
    }
    if (str == "LIMIT_MAKER") {
        return OrderType::LimitMaker;
    }
    throw std::invalid_argument(std::string("Unknown order type: ") + std::string(str));
}

TimeInForce time_in_force_from_string(std::string_view str) {
    if (str == "GTC") {
        return TimeInForce::GTC;
    }
    if (str == "IOC") {
        return TimeInForce::IOC;
    }
    if (str == "FOK") {
        return TimeInForce::FOK;
    }
    throw std::invalid_argument(std::string("Unknown time in force: ") + std::string(str));
}

OrderStatus order_status_from_string(std::string_view str) {
    if (str == "NEW") {
        return OrderStatus::New;
    }
    if (str == "PARTIALLY_FILLED") {
        return OrderStatus::PartiallyFilled;
    }
    if (str == "FILLED") {
        return OrderStatus::Filled;
    }
    if (str == "CANCELED") {
        return OrderStatus::Canceled;
    }
    if (str == "PENDING_CANCEL") {
        return OrderStatus::PendingCancel;
    }
    if (str == "REJECTED") {
        return OrderStatus::Rejected;
    }
    if (str == "EXPIRED") {
        return OrderStatus::Expired;
    }
    throw std::invalid_argument(std::string("Unknown order status: ") + std::string(str));
}

}  // namespace bintrade

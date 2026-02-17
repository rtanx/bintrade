#pragma once

#include <bintrade/core/types.hpp>

#include <string>
#include <vector>

namespace bintrade::models {

struct Balance {
    std::string asset;
    double free = 0.0;
    double locked = 0.0;

    [[nodiscard]] double total() const { return free + locked; }
};

struct AccountInfo {
    bool can_trade = false;
    bool can_withdraw = false;
    bool can_deposit = false;
    Timestamp update_time{};
    std::string account_type;
    std::vector<Balance> balances;
};

}  // namespace bintrade::models

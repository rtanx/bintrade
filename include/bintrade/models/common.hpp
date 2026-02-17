#pragma once

#include <bintrade/core/types.hpp>

#include <optional>
#include <string>

namespace bintrade::models {

struct SymbolInfo {
    Symbol symbol;
    std::string base_asset;
    std::string quote_asset;
    std::string status;
    Price min_price = 0.0;
    Price max_price = 0.0;
    Price tick_size = 0.0;
    Quantity min_qty = 0.0;
    Quantity max_qty = 0.0;
    Quantity step_size = 0.0;
};

struct Filter {
    std::string filter_type;
    std::optional<Price> min_price;
    std::optional<Price> max_price;
    std::optional<Quantity> min_qty;
    std::optional<Quantity> max_qty;
};

}  // namespace bintrade::models

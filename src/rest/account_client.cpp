#include "detail/json_parse.hpp"

#include <bintrade/rest/account_client.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace bintrade::rest {

models::AccountInfo AccountClient::get_account_info() {
    auto body = signed_get("/api/v3/account");
    auto json = detail::parse_response(200, body);
    return detail::parse_account_info(json);
}

std::vector<models::AccountTrade> AccountClient::get_account_trades(const Symbol& symbol, int limit) {
    Params params;
    params["symbol"] = symbol;
    params["limit"] = std::to_string(limit);
    auto body = signed_get("/api/v3/myTrades", std::move(params));
    auto json = detail::parse_response(200, body);
    std::vector<models::AccountTrade> trades;
    trades.reserve(json.size());
    for (const auto& item : json) {
        trades.push_back(detail::parse_account_trade(item));
    }
    return trades;
}

}  // namespace bintrade::rest

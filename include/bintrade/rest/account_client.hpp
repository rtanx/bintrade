#pragma once

#include <bintrade/models/account.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/rest/client.hpp>

#include <vector>

namespace bintrade::rest {

class AccountClient : public Client {
public:
    using Client::Client;

    [[nodiscard]] models::AccountInfo get_account_info();
    [[nodiscard]] std::vector<models::AccountTrade> get_account_trades(const Symbol& symbol, int limit = 500);
};

}  // namespace bintrade::rest

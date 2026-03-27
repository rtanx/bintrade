#pragma once

#include <bintrade/models/account.hpp>
#include <bintrade/models/trade.hpp>
#include <bintrade/rest/client.hpp>

#include <boost/asio/awaitable.hpp>
#include <vector>

namespace bintrade::rest {

class AccountClient : public Client {
public:
    using Client::Client;

    // -----------------------------------------------------------------------
    // Synchronous API
    // -----------------------------------------------------------------------
    [[nodiscard]] models::AccountInfo get_account_info();
    [[nodiscard]] std::vector<models::AccountTrade> get_account_trades(const Symbol& symbol, int limit = 500);

    // -----------------------------------------------------------------------
    // Asynchronous API
    // -----------------------------------------------------------------------
    [[nodiscard]] boost::asio::awaitable<models::AccountInfo> async_get_account_info();
    [[nodiscard]] boost::asio::awaitable<std::vector<models::AccountTrade>> async_get_account_trades(Symbol symbol, int limit = 500);
};

}  // namespace bintrade::rest

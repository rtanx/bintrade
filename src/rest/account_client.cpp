#include <bintrade/rest/account_client.hpp>

#include <stdexcept>

namespace bintrade::rest {

models::AccountInfo AccountClient::get_account_info() {
    // TODO: GET /api/v3/account
    throw std::runtime_error("AccountClient::get_account_info not implemented");
}

std::vector<models::AccountTrade> AccountClient::get_account_trades(const Symbol& /*symbol*/,
                                                                    int /*limit*/) {
    // TODO: GET /api/v3/myTrades
    throw std::runtime_error("AccountClient::get_account_trades not implemented");
}

}  // namespace bintrade::rest

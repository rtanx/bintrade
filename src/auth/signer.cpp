#include <bintrade/auth/credentials.hpp>
#include <bintrade/auth/signer.hpp>

#include <array>
#include <chrono>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace bintrade {

Signer::Signer(const Credentials& credentials) : credentials_(credentials) {}

std::string Signer::sign(std::string_view query) const {
    auto secret = credentials_.api_secret();

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;

    HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
         reinterpret_cast<const unsigned char*>(query.data()), query.size(), digest.data(),
         &digest_len);

    std::ostringstream hex_stream;
    hex_stream << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        hex_stream << std::setw(2) << static_cast<int>(digest[i]);
    }
    return hex_stream.str();
}

std::unordered_map<std::string, std::string>
Signer::sign_parameters(std::unordered_map<std::string, std::string> params) const {
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    params["timestamp"] = std::to_string(ms.count());

    // Build query string
    std::string query;
    for (const auto& [key, value] : params) {
        if (!query.empty()) {
            query += '&';
        }
        query += key;
        query += '=';
        query += value;
    }

    // Sign and add signature
    params["signature"] = sign(query);
    return params;
}

}  // namespace bintrade

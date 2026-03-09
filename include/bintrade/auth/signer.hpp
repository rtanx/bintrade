#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace bintrade {

class Credentials;

class Signer {
public:
    explicit Signer(const Credentials& credentials);

    [[nodiscard]] std::string sign(std::string_view query) const;

    [[nodiscard]] std::unordered_map<std::string, std::string> sign_parameters(std::unordered_map<std::string, std::string> params) const;

private:
    const Credentials& credentials_;
};

}  // namespace bintrade

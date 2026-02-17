#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace bintrade {

class Credentials {
public:
    Credentials() = default;
    Credentials(std::string api_key, std::string api_secret);

    Credentials(const Credentials&) = delete;
    Credentials& operator=(const Credentials&) = delete;

    Credentials(Credentials&&) noexcept;
    Credentials& operator=(Credentials&&) noexcept;

    ~Credentials();

    [[nodiscard]] bool has_credentials() const noexcept;
    [[nodiscard]] std::string_view api_key() const noexcept;
    [[nodiscard]] std::string_view api_secret() const noexcept;

    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace bintrade

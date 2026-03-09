#include <bintrade/auth/credentials.hpp>

#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace bintrade {

class Credentials::Impl {
public:
    Impl(std::string api_key, std::string api_secret) : api_key_(std::move(api_key)), api_secret_(std::move(api_secret)) {}

    ~Impl() { clear(); }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) noexcept = default;
    Impl& operator=(Impl&&) noexcept = default;

    [[nodiscard]] std::string_view api_key() const noexcept { return api_key_; }
    [[nodiscard]] std::string_view api_secret() const noexcept { return api_secret_; }

    void clear() {
        // Overwrite sensitive data before releasing
        if (!api_key_.empty()) {
            std::memset(api_key_.data(), 0, api_key_.size());
            api_key_.clear();
        }
        if (!api_secret_.empty()) {
            std::memset(api_secret_.data(), 0, api_secret_.size());
            api_secret_.clear();
        }
    }

private:
    std::string api_key_;
    std::string api_secret_;
};

Credentials::Credentials(std::string api_key, std::string api_secret) : impl_(std::make_unique<Impl>(std::move(api_key), std::move(api_secret))) {}

Credentials::Credentials(Credentials&&) noexcept = default;
Credentials& Credentials::operator=(Credentials&&) noexcept = default;
Credentials::~Credentials() = default;

bool Credentials::has_credentials() const noexcept {
    return impl_ != nullptr;
}

std::string_view Credentials::api_key() const noexcept {
    return impl_ ? impl_->api_key() : std::string_view{};
}

std::string_view Credentials::api_secret() const noexcept {
    return impl_ ? impl_->api_secret() : std::string_view{};
}

void Credentials::clear() {
    if (impl_) {
        impl_->clear();
    }
}

}  // namespace bintrade

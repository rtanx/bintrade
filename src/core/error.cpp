#include <bintrade/core/error.hpp>

#include <system_error>

namespace bintrade {

const char* ErrorCategory::name() const noexcept {
    return "bintrade";
}

std::string ErrorCategory::message(int ev) const {
    switch (static_cast<ErrorCode>(ev)) {
    case ErrorCode::Success:
        return "Success";
    case ErrorCode::ConnectionFailed:
        return "Connection failed";
    case ErrorCode::Timeout:
        return "Request timed out";
    case ErrorCode::InvalidApiKey:
        return "Invalid API key";
    case ErrorCode::InvalidSignature:
        return "Invalid signature";
    case ErrorCode::InvalidTimestamp:
        return "Invalid timestamp";
    case ErrorCode::RateLimitExceeded:
        return "Rate limit exceeded";
    case ErrorCode::InvalidSymbol:
        return "Invalid symbol";
    case ErrorCode::InvalidOrderType:
        return "Invalid order type";
    case ErrorCode::InvalidQuantity:
        return "Invalid quantity";
    case ErrorCode::InvalidPrice:
        return "Invalid price";
    case ErrorCode::InsufficientBalance:
        return "Insufficient balance";
    case ErrorCode::ParseError:
        return "JSON parse error";
    case ErrorCode::UnknownError:
        return "Unknown error";
    }
    return "Unknown error code";
}

const ErrorCategory& get_error_category() {
    static ErrorCategory instance;
    return instance;
}

std::error_code make_error_code(ErrorCode e) {
    return {static_cast<int>(e), get_error_category()};
}

Exception::Exception(std::string message)
    : code_(make_error_code(ErrorCode::UnknownError)), message_(std::move(message)) {}

Exception::Exception(std::error_code ec) : code_(ec), message_(ec.message()) {}

Exception::Exception(std::error_code ec, std::string message)
    : code_(ec), message_(std::move(message)) {}

const char* Exception::what() const noexcept {
    return message_.c_str();
}

std::error_code Exception::code() const noexcept {
    return code_;
}

ApiException::ApiException(int http_code, std::string message)
    : Exception(std::move(message)), http_code_(http_code) {}

int ApiException::http_code() const noexcept {
    return http_code_;
}

}  // namespace bintrade

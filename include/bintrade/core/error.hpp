#pragma once

#include <exception>
#include <string>
#include <system_error>
#include <type_traits>

namespace bintrade {

enum class ErrorCode {
    Success = 0,
    ConnectionFailed,
    Timeout,
    InvalidApiKey,
    InvalidSignature,
    InvalidTimestamp,
    RateLimitExceeded,
    InvalidSymbol,
    InvalidOrderType,
    InvalidQuantity,
    InvalidPrice,
    InsufficientBalance,
    ParseError,
    UnknownError
};

class ErrorCategory : public std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override;
    [[nodiscard]] std::string message(int ev) const override;
};

[[nodiscard]] const ErrorCategory& get_error_category();
[[nodiscard]] std::error_code make_error_code(ErrorCode e);

class Exception : public std::exception {
public:
    explicit Exception(std::string message);
    explicit Exception(std::error_code ec);
    Exception(std::error_code ec, std::string message);

    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] std::error_code code() const noexcept;

private:
    std::error_code code_;
    std::string message_;
};

class NetworkException : public Exception {
    using Exception::Exception;
};

class ApiException : public Exception {
public:
    ApiException(int http_code, std::string message);
    [[nodiscard]] int http_code() const noexcept;

private:
    int http_code_;
};

class AuthenticationException : public Exception {
    using Exception::Exception;
};

class ValidationException : public Exception {
    using Exception::Exception;
};

}  // namespace bintrade

template <>
struct std::is_error_code_enum<bintrade::ErrorCode> : std::true_type {};

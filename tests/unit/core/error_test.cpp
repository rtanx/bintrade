#include <bintrade/core/error.hpp>

#include <gtest/gtest.h>

#include <string>
#include <system_error>

namespace bintrade::test {

// ---------------------------------------------------------------------------
// ErrorCategory
// ---------------------------------------------------------------------------

TEST(ErrorCategoryTest, CategoryNameIsBintrade) {
    const auto& cat = get_error_category();
    EXPECT_STREQ(cat.name(), "bintrade");
}

TEST(ErrorCategoryTest, MessageSuccess) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::Success)), "Success");
}

TEST(ErrorCategoryTest, MessageConnectionFailed) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::ConnectionFailed)), "Connection failed");
}

TEST(ErrorCategoryTest, MessageTimeout) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::Timeout)), "Request timed out");
}

TEST(ErrorCategoryTest, MessageInvalidApiKey) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InvalidApiKey)), "Invalid API key");
}

TEST(ErrorCategoryTest, MessageInvalidSignature) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InvalidSignature)), "Invalid signature");
}

TEST(ErrorCategoryTest, MessageInvalidTimestamp) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InvalidTimestamp)), "Invalid timestamp");
}

TEST(ErrorCategoryTest, MessageRateLimitExceeded) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::RateLimitExceeded)), "Rate limit exceeded");
}

TEST(ErrorCategoryTest, MessageInvalidSymbol) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InvalidSymbol)), "Invalid symbol");
}

TEST(ErrorCategoryTest, MessageInvalidOrderType) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InvalidOrderType)), "Invalid order type");
}

TEST(ErrorCategoryTest, MessageInvalidQuantity) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InvalidQuantity)), "Invalid quantity");
}

TEST(ErrorCategoryTest, MessageInvalidPrice) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InvalidPrice)), "Invalid price");
}

TEST(ErrorCategoryTest, MessageInsufficientBalance) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::InsufficientBalance)), "Insufficient balance");
}

TEST(ErrorCategoryTest, MessageParseError) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::ParseError)), "JSON parse error");
}

TEST(ErrorCategoryTest, MessageUnknownError) {
    const auto& cat = get_error_category();
    EXPECT_EQ(cat.message(static_cast<int>(ErrorCode::UnknownError)), "Unknown error");
}

TEST(ErrorCategoryTest, MessageUnrecognizedCodeReturnsUnknown) {
    const auto& cat = get_error_category();
    EXPECT_FALSE(cat.message(9'999).empty());
}

// ---------------------------------------------------------------------------
// make_error_code
// ---------------------------------------------------------------------------

TEST(MakeErrorCodeTest, ProducesCorrectCategory) {
    auto ec = make_error_code(ErrorCode::Timeout);
    EXPECT_EQ(&ec.category(), &get_error_category());
}

TEST(MakeErrorCodeTest, ProducesCorrectValue) {
    auto ec = make_error_code(ErrorCode::InvalidApiKey);
    EXPECT_EQ(ec.value(), static_cast<int>(ErrorCode::InvalidApiKey));
}

TEST(MakeErrorCodeTest, ErrorCodeEnumIsErrorCodeEnum) {
    std::error_code ec = ErrorCode::ParseError;
    EXPECT_EQ(ec.value(), static_cast<int>(ErrorCode::ParseError));
}

// ---------------------------------------------------------------------------
// Exception -- string constructor
// ---------------------------------------------------------------------------

TEST(ExceptionTest, StringConstructorWhat) {
    Exception ex("something went wrong");
    EXPECT_STREQ(ex.what(), "something went wrong");
}

TEST(ExceptionTest, StringConstructorCodeIsUnknown) {
    Exception ex("msg");
    EXPECT_EQ(ex.code(), make_error_code(ErrorCode::UnknownError));
}

// ---------------------------------------------------------------------------
// Exception -- error_code constructor
// ---------------------------------------------------------------------------

TEST(ExceptionTest, ErrorCodeConstructorWhat) {
    auto ec = make_error_code(ErrorCode::Timeout);
    Exception ex(ec);
    EXPECT_STREQ(ex.what(), ec.message().c_str());
}

TEST(ExceptionTest, ErrorCodeConstructorCode) {
    auto ec = make_error_code(ErrorCode::RateLimitExceeded);
    Exception ex(ec);
    EXPECT_EQ(ex.code(), ec);
}

// ---------------------------------------------------------------------------
// Exception -- error_code + message constructor
// ---------------------------------------------------------------------------

TEST(ExceptionTest, ErrorCodeAndMessageConstructorWhat) {
    auto ec = make_error_code(ErrorCode::ConnectionFailed);
    Exception ex(ec, "custom message");
    EXPECT_STREQ(ex.what(), "custom message");
}

TEST(ExceptionTest, ErrorCodeAndMessageConstructorCode) {
    auto ec = make_error_code(ErrorCode::ConnectionFailed);
    Exception ex(ec, "custom message");
    EXPECT_EQ(ex.code(), ec);
}

// ---------------------------------------------------------------------------
// NetworkException
// ---------------------------------------------------------------------------

TEST(NetworkExceptionTest, DerivedFromException) {
    NetworkException ex("network failure");
    EXPECT_STREQ(ex.what(), "network failure");

    // Must be catchable as std::exception
    try {
        throw NetworkException("net error");
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "net error");
    }
}

TEST(NetworkExceptionTest, DerivedFromExceptionType) {
    try {
        throw NetworkException("net");
    } catch (const Exception& e) {
        SUCCEED();
        return;
    }
    FAIL() << "NetworkException not caught as Exception";
}

// ---------------------------------------------------------------------------
// ApiException
// ---------------------------------------------------------------------------

TEST(ApiExceptionTest, HttpCodeAccessor) {
    ApiException ex(429, "Too Many Requests");
    EXPECT_EQ(ex.http_code(), 429);
}

TEST(ApiExceptionTest, WhatReturnsMessage) {
    ApiException ex(400, "Bad Request");
    EXPECT_STREQ(ex.what(), "Bad Request");
}

TEST(ApiExceptionTest, DerivedFromException) {
    try {
        throw ApiException(500, "Internal Server Error");
    } catch (const Exception& e) {
        EXPECT_STREQ(e.what(), "Internal Server Error");
    }
}

TEST(ApiExceptionTest, DerivedFromStdException) {
    try {
        throw ApiException(401, "Unauthorized");
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "Unauthorized");
    }
}

// ---------------------------------------------------------------------------
// AuthenticationException
// ---------------------------------------------------------------------------

TEST(AuthenticationExceptionTest, StringConstructorWhat) {
    AuthenticationException ex("invalid API key");
    EXPECT_STREQ(ex.what(), "invalid API key");
}

TEST(AuthenticationExceptionTest, DerivedFromException) {
    try {
        throw AuthenticationException("auth failed");
    } catch (const Exception& e) {
        SUCCEED();
        return;
    }
    FAIL() << "AuthenticationException not caught as Exception";
}

TEST(AuthenticationExceptionTest, DerivedFromStdException) {
    try {
        throw AuthenticationException("auth");
    } catch (const std::exception&) {
        SUCCEED();
    }
}

// ---------------------------------------------------------------------------
// ValidationException
// ---------------------------------------------------------------------------

TEST(ValidationExceptionTest, StringConstructorWhat) {
    ValidationException ex("invalid symbol");
    EXPECT_STREQ(ex.what(), "invalid symbol");
}

TEST(ValidationExceptionTest, DerivedFromException) {
    try {
        throw ValidationException("bad input");
    } catch (const Exception& e) {
        SUCCEED();
        return;
    }
    FAIL() << "ValidationException not caught as Exception";
}

TEST(ValidationExceptionTest, DerivedFromStdException) {
    try {
        throw ValidationException("validation");
    } catch (const std::exception&) {
        SUCCEED();
    }
}

}  // namespace bintrade::test

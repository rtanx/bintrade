#include <bintrade/auth/credentials.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace bintrade::test {

// ---------------------------------------------------------------------------
// Default construction
// ---------------------------------------------------------------------------

TEST(CredentialsTest, DefaultConstructedHasNoCredentials) {
    Credentials c;
    EXPECT_FALSE(c.has_credentials());
}

TEST(CredentialsTest, DefaultConstructedApiKeyEmpty) {
    Credentials c;
    EXPECT_TRUE(c.api_key().empty());
}

TEST(CredentialsTest, DefaultConstructedApiSecretEmpty) {
    Credentials c;
    EXPECT_TRUE(c.api_secret().empty());
}

// ---------------------------------------------------------------------------
// Parameterized construction
// ---------------------------------------------------------------------------

TEST(CredentialsTest, ParameterizedConstructionHasCredentials) {
    Credentials c("my_api_key", "my_api_secret");
    EXPECT_TRUE(c.has_credentials());
}

TEST(CredentialsTest, ApiKeyAccessor) {
    Credentials c("key123", "secret456");
    EXPECT_EQ(c.api_key(), "key123");
}

TEST(CredentialsTest, ApiSecretAccessor) {
    Credentials c("key123", "secret456");
    EXPECT_EQ(c.api_secret(), "secret456");
}

TEST(CredentialsTest, EmptyStringsAreAccepted) {
    // Providing empty strings is allowed; has_credentials() reflects impl_ presence
    Credentials c("", "");
    EXPECT_TRUE(c.has_credentials());
    EXPECT_TRUE(c.api_key().empty());
    EXPECT_TRUE(c.api_secret().empty());
}

// ---------------------------------------------------------------------------
// Move constructor
// ---------------------------------------------------------------------------

TEST(CredentialsTest, MoveConstructorTransfersCredentials) {
    Credentials src("key", "secret");
    Credentials dst(std::move(src));

    EXPECT_TRUE(dst.has_credentials());
    EXPECT_EQ(dst.api_key(), "key");
    EXPECT_EQ(dst.api_secret(), "secret");
}

TEST(CredentialsTest, MoveConstructorLeavesSourceEmpty) {
    Credentials src("key", "secret");
    Credentials dst(std::move(src));

    // After move the source impl_ is nullptr
    EXPECT_FALSE(src.has_credentials());
}

// ---------------------------------------------------------------------------
// Move assignment
// ---------------------------------------------------------------------------

TEST(CredentialsTest, MoveAssignmentTransfersCredentials) {
    Credentials src("k2", "s2");
    Credentials dst;
    dst = std::move(src);

    EXPECT_TRUE(dst.has_credentials());
    EXPECT_EQ(dst.api_key(), "k2");
    EXPECT_EQ(dst.api_secret(), "s2");
}

TEST(CredentialsTest, MoveAssignmentLeavesSourceEmpty) {
    Credentials src("k3", "s3");
    Credentials dst;
    dst = std::move(src);

    EXPECT_FALSE(src.has_credentials());
}

TEST(CredentialsTest, SelfMoveAssignmentIsNoOp) {
    Credentials c("selfkey", "selfsecret");
    // Suppress -Wself-move in tests: we intentionally test self-move safety.
    // NOLINTNEXTLINE(clang-diagnostic-self-move)
    c = std::move(c);
    // After self-move the object may be in a valid but unspecified state;
    // the important thing is it does not crash.
}

// ---------------------------------------------------------------------------
// clear()
// ---------------------------------------------------------------------------

TEST(CredentialsTest, ClearZeroesApiKey) {
    Credentials c("clear_key", "clear_secret");
    c.clear();
    EXPECT_TRUE(c.api_key().empty());
}

TEST(CredentialsTest, ClearZeroesApiSecret) {
    Credentials c("clear_key", "clear_secret");
    c.clear();
    EXPECT_TRUE(c.api_secret().empty());
}

TEST(CredentialsTest, HasCredentialsRemainsAfterClear) {
    // impl_ still exists after clear(); has_credentials() stays true
    Credentials c("k", "s");
    c.clear();
    EXPECT_TRUE(c.has_credentials());
}

TEST(CredentialsTest, ClearOnDefaultConstructedIsNoOp) {
    Credentials c;
    // Must not throw or crash when impl_ is null
    EXPECT_NO_THROW(c.clear());
}

// ---------------------------------------------------------------------------
// Copy semantics deleted
// ---------------------------------------------------------------------------
// Verified at compile time: Credentials(const Credentials&) = delete.
// No runtime test needed; attempting to copy fails to compile.

}  // namespace bintrade::test

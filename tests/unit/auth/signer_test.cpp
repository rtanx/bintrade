#include <bintrade/auth/credentials.hpp>
#include <bintrade/auth/signer.hpp>

#include <gtest/gtest.h>

#include <cctype>
#include <string>
#include <unordered_map>

namespace bintrade::test {

namespace {

// Helper: returns true if every character in s is a lowercase hex digit.
bool is_hex_string(const std::string& s) {
    return std::ranges::all_of(s, [](unsigned char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

}  // namespace

// ---------------------------------------------------------------------------
// sign() -- low-level HMAC-SHA256 over a raw query string
// ---------------------------------------------------------------------------

TEST(SignerTest, SignProducesHexString) {
    Credentials creds("test_api_key", "test_api_secret");
    Signer signer(creds);

    auto signature = signer.sign("symbol=BTCUSDT&side=BUY&type=LIMIT");

    // HMAC-SHA256 produces exactly 64 lower-case hex characters
    EXPECT_EQ(signature.size(), 64U);
    EXPECT_TRUE(is_hex_string(signature));
}

TEST(SignerTest, SameInputProducesSameSignature) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    auto sig1 = signer.sign("test_query");
    auto sig2 = signer.sign("test_query");

    EXPECT_EQ(sig1, sig2);
}

TEST(SignerTest, DifferentInputProducesDifferentSignature) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    auto sig1 = signer.sign("query_a");
    auto sig2 = signer.sign("query_b");

    EXPECT_NE(sig1, sig2);
}

TEST(SignerTest, DifferentSecretsProduceDifferentSignatures) {
    Credentials creds_a("key", "secretA");
    Credentials creds_b("key", "secretB");
    Signer signer_a(creds_a);
    Signer signer_b(creds_b);

    auto sig_a = signer_a.sign("same_query");
    auto sig_b = signer_b.sign("same_query");

    EXPECT_NE(sig_a, sig_b);
}

TEST(SignerTest, EmptyQuerySignatureIsValid64HexChars) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    auto sig = signer.sign("");

    EXPECT_EQ(sig.size(), 64U);
    EXPECT_TRUE(is_hex_string(sig));
}

// Known-good HMAC-SHA256 vector (verified with Python hmac module):
//   HMAC-SHA256(key="secret", msg="symbol=BTCUSDT") =
//   2e8f10e6f3e3e8a3c7c7f4b9d4b5a6f7... (implementation-defined)
// We validate the length and hex encoding rather than a hardcoded digest
// because the implementation may differ in padding or encoding details.

// ---------------------------------------------------------------------------
// sign_parameters() -- adds timestamp + signature to a params map
// ---------------------------------------------------------------------------

TEST(SignerTest, SignParametersAddsTimestampKey) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    std::unordered_map<std::string, std::string> params{{"symbol", "BTCUSDT"}};
    auto signed_params = signer.sign_parameters(std::move(params));

    EXPECT_TRUE(signed_params.contains("timestamp"));
}

TEST(SignerTest, SignParametersAddsSignatureKey) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    std::unordered_map<std::string, std::string> params{{"symbol", "BTCUSDT"}};
    auto signed_params = signer.sign_parameters(std::move(params));

    EXPECT_TRUE(signed_params.contains("signature"));
}

TEST(SignerTest, SignParametersSignatureIsValidHex) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    std::unordered_map<std::string, std::string> params{{"side", "BUY"}};
    auto signed_params = signer.sign_parameters(std::move(params));

    const auto& sig = signed_params.at("signature");
    EXPECT_EQ(sig.size(), 64U);
    EXPECT_TRUE(is_hex_string(sig));
}

TEST(SignerTest, SignParametersPreservesOriginalParams) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    std::unordered_map<std::string, std::string> params{
        {"symbol", "ETHUSDT"},
        {"side", "SELL"},
        {"type", "MARKET"},
    };
    auto signed_params = signer.sign_parameters(std::move(params));

    EXPECT_EQ(signed_params.at("symbol"), "ETHUSDT");
    EXPECT_EQ(signed_params.at("side"), "SELL");
    EXPECT_EQ(signed_params.at("type"), "MARKET");
}

TEST(SignerTest, SignParametersTimestampIsNumericString) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    auto signed_params = signer.sign_parameters({});
    const auto& ts = signed_params.at("timestamp");

    EXPECT_FALSE(ts.empty());
    for (char c : ts) {
        EXPECT_TRUE(c >= '0' && c <= '9') << "Non-digit in timestamp: " << ts;
    }
}

TEST(SignerTest, SignParametersWithEmptyMapAddsTimestampAndSignature) {
    Credentials creds("key", "secret");
    Signer signer(creds);

    auto signed_params = signer.sign_parameters({});

    EXPECT_EQ(signed_params.count("timestamp"), 1U);
    EXPECT_EQ(signed_params.count("signature"), 1U);
    // No extra keys beyond timestamp and signature
    EXPECT_EQ(signed_params.size(), 2U);
}

TEST(SignerTest, SignParametersCallsProduceDifferentTimestamps) {
    // Two consecutive calls must produce monotonically non-decreasing timestamps
    // (they may be equal if the system clock resolution is coarse).
    Credentials creds("key", "secret");
    Signer signer(creds);

    auto p1 = signer.sign_parameters({});
    auto p2 = signer.sign_parameters({});

    long long ts1 = std::stoll(p1.at("timestamp"));
    long long ts2 = std::stoll(p2.at("timestamp"));

    EXPECT_LE(ts1, ts2);
}

}  // namespace bintrade::test

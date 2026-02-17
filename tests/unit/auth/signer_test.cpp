#include <bintrade/auth/credentials.hpp>
#include <bintrade/auth/signer.hpp>

#include <gtest/gtest.h>

namespace bintrade::test {

TEST(SignerTest, SignProducesHexString) {
    Credentials creds("test_api_key", "test_api_secret");
    Signer signer(creds);

    auto signature = signer.sign("symbol=BTCUSDT&side=BUY&type=LIMIT");

    // HMAC-SHA256 produces a 64 hex character string
    EXPECT_EQ(signature.size(), 64);

    // All characters should be hex
    for (char c : signature) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
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

}  // namespace bintrade::test

#include <gtest/gtest.h>

#include <string_view>

#include "iso8583/mti.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

TEST(Mti, ParsesFourDigits) {
    const auto mti = Mti::parse("0210");
    ASSERT_TRUE(mti);
    EXPECT_EQ(mti->view(), "0210");
    EXPECT_EQ(*mti, Mti{"0210"});
}

TEST(Mti, ExposesEachDigit) {
    constexpr Mti mti{"1234"};
    static_assert(mti.version() == 1 && mti.message_class() == 2);
    static_assert(mti.function() == 3 && mti.origin() == 4);
    EXPECT_EQ(mti.view(), "1234");
}

TEST(Mti, EqualityComparesEveryDigit) {
    EXPECT_EQ(Mti{"0200"}, Mti::parse("0200").value());
    EXPECT_NE(Mti{"0200"}, Mti{"1200"});
    EXPECT_NE(Mti{"0200"}, Mti{"0100"});
    EXPECT_NE(Mti{"0200"}, Mti{"0210"});
    EXPECT_NE(Mti{"0200"}, Mti{"0201"});
}

class MtiRejects : public testing::TestWithParam<std::string_view> {};

TEST_P(MtiRejects, TextThatIsNotFourDigits) {
    const auto mti = Mti::parse(GetParam());
    ASSERT_FALSE(mti);
    EXPECT_EQ(mti.error(), Error{Errc::bad_mti});
}

INSTANTIATE_TEST_SUITE_P(Malformed, MtiRejects,
                         testing::Values("", "020", "02000", "02A0", " 200", "02-0"));

TEST(Mti, DescribesCommonMessageTypes) {
    EXPECT_EQ(describe(Mti{"0100"}), "authorization request (origin acquirer, ISO 8583:1987)");
    EXPECT_EQ(describe(Mti{"0200"}), "financial request (origin acquirer, ISO 8583:1987)");
    EXPECT_EQ(describe(Mti{"0210"}), "financial request response (origin acquirer, ISO 8583:1987)");
    EXPECT_EQ(describe(Mti{"0421"}),
              "reversal or chargeback advice (origin acquirer repeat, ISO 8583:1987)");
    EXPECT_EQ(describe(Mti{"1804"}), "network management request (origin other, ISO 8583:1993)");
}

TEST(Mti, DescribesReservedAndPrivateDigits) {
    EXPECT_EQ(describe(Mti{"9999"}),
              "reserved class reserved function (origin reserved, private use)");
}

} // namespace
} // namespace iso8583

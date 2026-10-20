#include <gtest/gtest.h>

#include <string_view>

#include "iso8583/bytes.hpp"

namespace iso8583 {
namespace {

TEST(Hex, EncodesUpperCaseWithoutSeparators) {
    const Bytes bytes{std::byte{0x00}, std::byte{0x0F}, std::byte{0xA5}, std::byte{0xFF}};
    EXPECT_EQ(to_hex(bytes), "000FA5FF");
    EXPECT_EQ(to_hex({}), "");
}

TEST(Hex, DecodesEitherCaseAndSkipsWhitespace) {
    const auto bytes = from_hex("30 32\t30\r\n30 aB");
    ASSERT_TRUE(bytes);
    EXPECT_EQ(to_hex(*bytes), "30323030AB");
}

TEST(Hex, EmptyTextIsNoBytes) {
    const auto bytes = from_hex("");
    ASSERT_TRUE(bytes);
    EXPECT_TRUE(bytes->empty());
}

class HexRejects : public testing::TestWithParam<std::string_view> {};

TEST_P(HexRejects, MalformedText) {
    EXPECT_FALSE(from_hex(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(Malformed, HexRejects,
                         testing::Values("303", "3G", "0x30", "30-31", "3 0 3"));

} // namespace
} // namespace iso8583

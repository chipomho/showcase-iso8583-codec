#include <gtest/gtest.h>

#include <array>
#include <span>
#include <string>
#include <vector>

#include "fixtures.hpp"
#include "iso8583/codec.hpp"
#include "iso8583/print.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

using test::ascii;
using test::hex;
using test::join;

TEST(Decode, FinancialRequestFromHandAssembledBytes) {
    const auto decoded = decode(test::financial_request_wire());
    ASSERT_TRUE(decoded) << to_string(decoded.error());
    EXPECT_EQ(*decoded, test::financial_request());
}

TEST(Decode, FinancialResponseFromHandAssembledBytes) {
    const auto decoded = decode(test::financial_response_wire());
    ASSERT_TRUE(decoded) << to_string(decoded.error());
    EXPECT_EQ(*decoded, test::financial_response());
}

TEST(Decode, CustomTableReadsLllvarTextAndBinary) {
    const Bytes wire = join(
        {ascii("0200"), hex("0000000000010200"), ascii("005R01=Y"), ascii("003"), hex("9F2602")});
    const auto decoded = decode(wire, test::extended_table);
    ASSERT_TRUE(decoded) << to_string(decoded.error());
    EXPECT_EQ(decoded->text(48), "R01=Y");
    ASSERT_TRUE(decoded->binary(55));
    EXPECT_EQ(to_hex(*decoded->binary(55)), "9F2602");
}

TEST(Decode, AcceptsASecondaryBitmapWithNoSecondaryFields) {
    const auto decoded =
        decode(join({ascii("0800"), hex("8000000000000000"), hex("0000000000000000")}));
    ASSERT_TRUE(decoded) << to_string(decoded.error());
    EXPECT_TRUE(decoded->fields().empty());
}

TEST(Decode, AcceptsAnEmptyVariableLengthField) {
    const auto decoded = decode(join({ascii("0200"), hex("4000000000000000"), ascii("00")}));
    ASSERT_TRUE(decoded) << to_string(decoded.error());
    EXPECT_EQ(decoded->text(2), "");
}

struct DecodeFailure {
    std::string name;
    Bytes input;
    Error expected;
};

void PrintTo(const DecodeFailure& failure, std::ostream* os) {
    *os << failure.name;
}

class DecodeRejects : public testing::TestWithParam<DecodeFailure> {};

TEST_P(DecodeRejects, MalformedInput) {
    const DecodeFailure& failure = GetParam();
    const auto decoded = decode(failure.input);
    ASSERT_FALSE(decoded);
    EXPECT_EQ(decoded.error(), failure.expected);
}

// Offsets: the MTI is bytes 0-3 and the primary bitmap 4-11, so the first
// field of a primary-only message starts at offset 12.
const std::vector<DecodeFailure> decode_failures{
    {"empty_input", {}, {Errc::truncated, 0, 0}},
    {"short_mti", ascii("020"), {Errc::truncated, 0, 0}},
    {"letter_in_mti", join({ascii("02A0"), hex("0000000000000000")}), {Errc::bad_mti, 0, 0}},
    {"truncated_primary_bitmap", join({ascii("0200"), hex("40000000")}), {Errc::truncated, 0, 4}},
    {"truncated_secondary_bitmap",
     join({ascii("0200"), hex("C000000000000000"), hex("0000")}),
     {Errc::truncated, 0, 12}},
    {"field_not_in_table",
     join({ascii("0200"), hex("0800000000000000")}),
     {Errc::unknown_field, 5, 12}},
    {"tertiary_bitmap_indicator",
     join({ascii("0200"), hex("8000000000000000"), hex("8000000000000000")}),
     {Errc::unknown_field, 65, 20}},
    {"truncated_length_prefix",
     join({ascii("0200"), hex("4000000000000000"), ascii("1")}),
     {Errc::truncated, 2, 12}},
    {"letter_in_length_prefix",
     join({ascii("0200"), hex("4000000000000000"), ascii("1X4111111111111111")}),
     {Errc::bad_length_prefix, 2, 12}},
    {"length_prefix_over_maximum",
     join({ascii("0200"), hex("4000000000000000"), ascii("20"), ascii(std::string(20, '4'))}),
     {Errc::length_exceeds_maximum, 2, 12}},
    {"truncated_pan",
     join({ascii("0200"), hex("4000000000000000"), ascii("16411111")}),
     {Errc::truncated, 2, 12}},
    {"truncated_fixed_field",
     join({ascii("0200"), hex("1000000000000000"), ascii("00000000")}),
     {Errc::truncated, 4, 12}},
    {"decimal_point_in_amount",
     join({ascii("0200"), hex("1000000000000000"), ascii("0000000025.0")}),
     {Errc::non_numeric, 4, 22}},
    {"punctuation_in_an",
     join({ascii("0210"), hex("0000000002000000"), ascii("0!")}),
     {Errc::invalid_character, 39, 13}},
    {"control_character_in_ans",
     join({ascii("0200"), hex("0000000000800000"), ascii("TERM"), hex("01"), ascii("001")}),
     {Errc::invalid_character, 41, 16}},
    {"truncated_pin_block",
     join({ascii("0200"), hex("0000000000001000"), hex("9F3A510CE2")}),
     {Errc::truncated, 52, 12}},
    {"trailing_data",
     join({ascii("0200"), hex("2000000000000000"), ascii("000000X")}),
     {Errc::trailing_data, 0, 18}},
};

INSTANTIATE_TEST_SUITE_P(Inputs, DecodeRejects, testing::ValuesIn(decode_failures),
                         [](const auto& param_info) { return param_info.param.name; });

TEST(Decode, EveryTruncationOfAValidMessageIsReportedAsTruncated) {
    const Bytes wire = test::financial_request_wire();
    const std::span<const std::byte> whole{wire};
    for (std::size_t length = 0; length < wire.size(); ++length) {
        const auto decoded = decode(whole.first(length));
        ASSERT_FALSE(decoded) << "decoded a message cut to " << length << " bytes";
        EXPECT_EQ(decoded.error().code, Errc::truncated) << "cut to " << length << " bytes";
    }
}

// Flips bits throughout a valid message. Whatever decode makes of the result,
// it must not crash (the sanitizer build checks for out-of-bounds reads), and
// anything it accepts must be a message encode accepts too.
TEST(Decode, CorruptedInputIsRejectedOrDecodesToAValidMessage) {
    const Bytes wire = test::financial_request_wire();
    constexpr std::array<unsigned, 4> masks{0x01, 0x10, 0x80, 0xFF};
    int accepted = 0;
    for (std::size_t i = 0; i < wire.size(); ++i) {
        for (const unsigned mask : masks) {
            Bytes corrupted = wire;
            corrupted[i] ^= static_cast<std::byte>(mask);
            const auto decoded = decode(corrupted);
            if (decoded) {
                ++accepted;
                const auto reencoded = encode(*decoded);
                ASSERT_TRUE(reencoded)
                    << "byte " << i << " mask " << mask << ": " << to_string(reencoded.error());
                EXPECT_EQ(decode(*reencoded).value(), *decoded);
            }
        }
    }
    // Flipping bits inside a b field leaves a valid message, so some must pass.
    EXPECT_GT(accepted, 0);
}

} // namespace
} // namespace iso8583

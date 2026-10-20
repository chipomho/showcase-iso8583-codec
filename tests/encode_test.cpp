#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
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

TEST(Encode, FinancialRequestMatchesHandAssembledBytes) {
    const auto encoded = encode(test::financial_request());
    ASSERT_TRUE(encoded) << to_string(encoded.error());
    EXPECT_EQ(to_hex(*encoded), to_hex(test::financial_request_wire()));
}

TEST(Encode, ResponseWithNoHighFieldsHasOnlyThePrimaryBitmap) {
    const auto encoded = encode(test::financial_response());
    ASSERT_TRUE(encoded) << to_string(encoded.error());
    EXPECT_EQ(to_hex(*encoded), to_hex(test::financial_response_wire()));
}

TEST(Encode, MessageWithNoFieldsIsMtiAndAnEmptyBitmap) {
    const auto encoded = encode(Message{Mti{"0800"}});
    ASSERT_TRUE(encoded);
    EXPECT_EQ(to_hex(*encoded), to_hex(join({ascii("0800"), hex("0000000000000000")})));
}

TEST(Encode, PadsShortFixedLengthText) {
    Message message{Mti{"0200"}};
    message.set(4, "2500").set(39, "0").set(41, "T1");

    const auto encoded = encode(message);
    ASSERT_TRUE(encoded) << to_string(encoded.error());
    EXPECT_EQ(to_hex(*encoded), to_hex(join({
                                    ascii("0200"),
                                    hex("1000000002800000"), // fields 4, 39, 41
                                    ascii("000000002500"),   // n: leading zeros
                                    ascii("0 "),             // an: trailing spaces
                                    ascii("T1      "),       // ans: trailing spaces
                                })));
}

TEST(Encode, AcceptsSpaceInAnAndAnyPrintableCharacterInAns) {
    Message message{Mti{"0210"}};
    message.set(38, "AB 12 ").set(42, "Shop #1 (Melb.)");
    EXPECT_TRUE(encode(message));
}

// The characters at and just beyond each edge of the n, an and ans sets.
TEST(Encode, CharacterSetBoundaries) {
    struct Probe {
        int field;
        char c;
        bool allowed;
    };
    constexpr std::array probes{
        // n: field 4
        Probe{4, '0', true}, Probe{4, '9', true}, Probe{4, '/', false}, Probe{4, ':', false},
        // an: field 38
        Probe{38, '0', true}, Probe{38, '9', true}, Probe{38, 'A', true}, Probe{38, 'Z', true},
        Probe{38, 'a', true}, Probe{38, 'z', true}, Probe{38, ' ', true}, Probe{38, '/', false},
        Probe{38, ':', false}, Probe{38, '@', false}, Probe{38, '[', false}, Probe{38, '`', false},
        Probe{38, '{', false},
        // ans: field 41
        Probe{41, ' ', true}, Probe{41, '~', true}, Probe{41, '\x1F', false},
        Probe{41, '\x7F', false}};

    for (const Probe& probe : probes) {
        SCOPED_TRACE(testing::Message()
                     << "field " << probe.field << ", char code " << static_cast<int>(probe.c));
        Message message{Mti{"0200"}};
        message.set(probe.field, std::string(1, probe.c));
        const auto encoded = encode(message);
        if (probe.allowed) {
            EXPECT_TRUE(encoded);
        } else {
            ASSERT_FALSE(encoded);
            const Errc expected = probe.field == 4 ? Errc::non_numeric : Errc::invalid_character;
            EXPECT_EQ(encoded.error(), (Error{expected, probe.field}));
        }
    }
}

TEST(Encode, EmptyVariableLengthFieldIsJustItsPrefix) {
    Message message{Mti{"0200"}};
    message.set(2, "");
    const auto encoded = encode(message);
    ASSERT_TRUE(encoded);
    EXPECT_EQ(to_hex(*encoded),
              to_hex(join({ascii("0200"), hex("4000000000000000"), ascii("00")})));
}

TEST(Encode, CustomTableCarriesLllvarTextAndBinary) {
    Message message{Mti{"0200"}};
    message.set(48, "R01=Y").set(55, hex("9F2602ABCD"));

    const auto encoded = encode(message, test::extended_table);
    ASSERT_TRUE(encoded) << to_string(encoded.error());
    EXPECT_EQ(to_hex(*encoded), to_hex(join({
                                    ascii("0200"),
                                    hex("0000000000010200"), // fields 48 and 55
                                    ascii("005R01=Y"),
                                    ascii("005"),
                                    hex("9F2602ABCD"),
                                })));
}

TEST(Encode, CustomTableRejectsOverlongBinary) {
    Message message{Mti{"0200"}};
    message.set(55, Bytes(256));
    EXPECT_EQ(encode(message, test::extended_table).error(),
              (Error{Errc::length_exceeds_maximum, 55}));
}

TEST(Encode, ReportsTheFirstBadFieldInWireOrder) {
    Message message{Mti{"0200"}};
    message.set(4, "bad").set(3, "bad");
    EXPECT_EQ(encode(message).error(), (Error{Errc::non_numeric, 3}));
}

struct EncodeFailure {
    std::string name;
    int field;
    FieldValue value;
    Error expected;
};

void PrintTo(const EncodeFailure& failure, std::ostream* os) {
    *os << failure.name;
}

class EncodeRejects : public testing::TestWithParam<EncodeFailure> {};

TEST_P(EncodeRejects, InvalidField) {
    const EncodeFailure& failure = GetParam();
    Message message{Mti{"0200"}};
    std::visit([&](const auto& value) { message.set(failure.field, value); }, failure.value);

    const auto encoded = encode(message);
    ASSERT_FALSE(encoded);
    EXPECT_EQ(encoded.error(), failure.expected);
}

const std::vector<EncodeFailure> encode_failures{
    {"field_not_in_table", 5, std::string{"1"}, {Errc::unknown_field, 5}},
    {"primary_bitmap_indicator", 1, std::string{"1"}, {Errc::unknown_field, 1}},
    {"tertiary_bitmap_indicator", 65, Bytes(8), {Errc::unknown_field, 65}},
    {"text_for_binary_field", 52, std::string{"12345678"}, {Errc::type_mismatch, 52}},
    {"bytes_for_text_field", 3, Bytes(6), {Errc::type_mismatch, 3}},
    {"pan_too_long", 2, std::string(20, '4'), {Errc::length_exceeds_maximum, 2}},
    {"fixed_text_too_long", 3, std::string{"0000000"}, {Errc::length_exceeds_maximum, 3}},
    {"pin_block_too_short", 52, Bytes(7), {Errc::wrong_length, 52}},
    {"mac_too_long", 128, Bytes(9), {Errc::wrong_length, 128}},
    {"decimal_point_in_amount", 4, std::string{"25.00"}, {Errc::non_numeric, 4}},
    {"space_in_pan", 2, std::string{"4111 1111"}, {Errc::non_numeric, 2}},
    {"punctuation_in_an", 39, std::string{"0!"}, {Errc::invalid_character, 39}},
    {"control_character_in_ans", 41, std::string{"TERM\x01"}, {Errc::invalid_character, 41}},
    {"delete_in_ans", 42, std::string{"\x7F"}, {Errc::invalid_character, 42}},
};

INSTANTIATE_TEST_SUITE_P(Fields, EncodeRejects, testing::ValuesIn(encode_failures),
                         [](const auto& param_info) { return param_info.param.name; });

} // namespace
} // namespace iso8583

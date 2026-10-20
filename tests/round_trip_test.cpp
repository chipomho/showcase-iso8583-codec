#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <string>
#include <string_view>

#include "fixtures.hpp"
#include "iso8583/codec.hpp"
#include "iso8583/print.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

TEST(RoundTrip, FixtureMessagesSurviveEncodeThenDecode) {
    for (const Message& original : {test::financial_request(), test::financial_response()}) {
        const auto encoded = encode(original);
        ASSERT_TRUE(encoded) << to_string(encoded.error());
        const auto decoded = decode(*encoded);
        ASSERT_TRUE(decoded) << to_string(decoded.error());
        EXPECT_EQ(*decoded, original);
    }
}

TEST(RoundTrip, FixtureBytesSurviveDecodeThenEncode) {
    for (const Bytes& wire : {test::financial_request_wire(), test::financial_response_wire()}) {
        const auto decoded = decode(wire);
        ASSERT_TRUE(decoded) << to_string(decoded.error());
        const auto encoded = encode(*decoded);
        ASSERT_TRUE(encoded) << to_string(encoded.error());
        EXPECT_EQ(to_hex(*encoded), to_hex(wire));
    }
}

// Builds random valid messages from a field table. Seeded, and it uses the
// engine's raw output rather than a distribution (whose results differ
// between standard libraries), so every run on every platform sees the same
// messages.
class MessageGenerator {
public:
    explicit MessageGenerator(std::uint32_t seed) : engine_{seed} {}

    Message next(const FieldTable& table) {
        Message message = random_mti();
        for (const FieldSpec& spec : table.specs()) {
            if (below(2) == 0) {
                continue;
            }
            const std::size_t length = spec.length_type == LengthType::fixed
                                           ? spec.max_length
                                           : below(spec.max_length + 1);
            if (spec.format == Format::b) {
                message.set(spec.number, random_bytes(length));
            } else {
                message.set(spec.number, random_text(spec.format, length));
            }
        }
        return message;
    }

private:
    std::size_t below(std::size_t bound) { return static_cast<std::size_t>(engine_() % bound); }

    Message random_mti() {
        std::string digits;
        for (int i = 0; i < 4; ++i) {
            digits += static_cast<char>('0' + below(10));
        }
        return Message{Mti::parse(digits).value()};
    }

    std::string random_text(Format format, std::size_t length) {
        constexpr std::string_view digits = "0123456789";
        constexpr std::string_view alphanumeric =
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz ";
        constexpr std::string_view printable =
            " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
            "abcdefghijklmnopqrstuvwxyz{|}~";
        const std::string_view alphabet = format == Format::n    ? digits
                                          : format == Format::an ? alphanumeric
                                                                 : printable;
        std::string text;
        for (std::size_t i = 0; i < length; ++i) {
            text += alphabet[below(alphabet.size())];
        }
        return text;
    }

    Bytes random_bytes(std::size_t length) {
        Bytes bytes;
        for (std::size_t i = 0; i < length; ++i) {
            bytes.push_back(static_cast<std::byte>(below(256)));
        }
        return bytes;
    }

    std::mt19937 engine_;
};

void expect_round_trip(const Message& original, const FieldTable& table) {
    const auto encoded = encode(original, table);
    ASSERT_TRUE(encoded) << to_string(encoded.error()) << '\n' << original;
    const auto decoded = decode(*encoded, table);
    ASSERT_TRUE(decoded) << to_string(decoded.error()) << '\n' << original;
    EXPECT_EQ(*decoded, original);
}

TEST(RoundTrip, RandomMessagesFromThe1987Table) {
    MessageGenerator generator{8583};
    for (int i = 0; i < 1000; ++i) {
        expect_round_trip(generator.next(fields_1987), fields_1987);
    }
}

TEST(RoundTrip, RandomMessagesFromATableWithLllvarFields) {
    MessageGenerator generator{1987};
    for (int i = 0; i < 300; ++i) {
        expect_round_trip(generator.next(test::extended_table), test::extended_table);
    }
}

} // namespace
} // namespace iso8583

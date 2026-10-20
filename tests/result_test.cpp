#include <gtest/gtest.h>

#include <array>
#include <set>
#include <string>
#include <utility>
#include <variant>

#include "iso8583/result.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

constexpr std::array all_codes{
    Errc::truncated,     Errc::bad_mti,       Errc::bad_length_prefix, Errc::length_exceeds_maximum,
    Errc::wrong_length,  Errc::non_numeric,   Errc::invalid_character, Errc::unknown_field,
    Errc::type_mismatch, Errc::trailing_data,
};

TEST(Errc, EveryCodeHasItsOwnDescription) {
    std::set<std::string_view> seen;
    for (const Errc code : all_codes) {
        const std::string_view text = describe(code);
        EXPECT_FALSE(text.empty());
        EXPECT_TRUE(seen.insert(text).second) << "duplicate description: " << text;
    }
}

TEST(Errc, ValueOutsideTheEnumStillDescribes) {
    EXPECT_EQ(describe(static_cast<Errc>(200)), "unrecognised error code");
}

TEST(Error, ToStringNamesTheFieldAndOffset) {
    EXPECT_EQ(to_string(Error{Errc::non_numeric, 4, 31}),
              "field 4: numeric field contains a non-digit (offset 31)");
}

TEST(Error, ToStringLeavesOutWhatIsNotKnown) {
    EXPECT_EQ(to_string(Error{Errc::bad_mti}), "message type indicator is not four digits");
}

TEST(Error, EqualityComparesEveryMember) {
    const Error error{Errc::truncated, 2, 12};
    EXPECT_EQ(error, (Error{Errc::truncated, 2, 12}));
    EXPECT_NE(error, (Error{Errc::bad_length_prefix, 2, 12}));
    EXPECT_NE(error, (Error{Errc::truncated, 3, 12}));
    EXPECT_NE(error, (Error{Errc::truncated, 2, 13}));
    EXPECT_NE(error, (Error{Errc::truncated, 2}));
}

TEST(Result, HoldsAValue) {
    const Result<std::string> result{std::string{"0200"}};
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "0200");
    EXPECT_EQ(result.value(), "0200");
    EXPECT_EQ(result->size(), 4U);
}

TEST(Result, HoldsAnError) {
    const Result<std::string> result{Error{Errc::truncated, 2, 5}};
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), (Error{Errc::truncated, 2, 5}));
}

TEST(Result, AllowsTheValueToBeModifiedInPlace) {
    Result<std::string> result{std::string{"02"}};
    result->append("0");
    *result += "0";
    EXPECT_EQ(result.value(), "0200");
}

TEST(Result, MovesTheValueOut) {
    Result<std::string> result{std::string(64, 'x')};
    const std::string taken = std::move(result).value();
    EXPECT_EQ(taken.size(), 64U);
}

TEST(Result, AskingAnErrorForItsValueIsAProgrammingError) {
    Result<std::string> result{Error{Errc::truncated}};
    EXPECT_THROW(static_cast<void>(result.value()), std::bad_variant_access);
}

} // namespace
} // namespace iso8583

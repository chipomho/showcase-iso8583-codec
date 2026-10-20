#include <gtest/gtest.h>

#include <array>
#include <initializer_list>
#include <span>
#include <string_view>
#include <vector>

#include "fixtures.hpp"
#include "iso8583/field_spec.hpp"
#include "iso8583/fields_1987.hpp"

namespace iso8583 {
namespace {

bool valid(std::initializer_list<FieldSpec> specs) {
    return is_valid_table(std::span<const FieldSpec>{specs.begin(), specs.size()});
}

constexpr FieldSpec fixed_n(int number, std::size_t length = 6) {
    return {number, Format::n, LengthType::fixed, length, Display::clear, "test"};
}

TEST(Fields1987, DefinesTheSupportedSubset) {
    std::vector<int> numbers;
    for (const FieldSpec& spec : fields_1987.specs()) {
        numbers.push_back(spec.number);
    }
    EXPECT_EQ(numbers, (std::vector<int>{2, 3, 4, 7, 11, 12, 13, 37, 38, 39, 41, 42, 49, 52, 128}));
}

TEST(Fields1987, MatchesTheStandardsNotation) {
    struct Case {
        int number;
        std::string_view notation;
    };
    constexpr std::array cases{
        Case{2, "n..19"}, Case{3, "n6"},     Case{4, "n12"},   Case{7, "n10"},  Case{11, "n6"},
        Case{12, "n6"},   Case{13, "n4"},    Case{37, "an12"}, Case{38, "an6"}, Case{39, "an2"},
        Case{41, "ans8"}, Case{42, "ans15"}, Case{49, "n3"},   Case{52, "b8"},  Case{128, "b8"},
    };
    for (const Case& c : cases) {
        const FieldSpec* spec = fields_1987.find(c.number);
        ASSERT_NE(spec, nullptr) << "field " << c.number;
        EXPECT_EQ(notation(*spec), c.notation) << "field " << c.number;
    }
}

TEST(Fields1987, MasksThePanAndRedactsThePinBlock) {
    EXPECT_EQ(fields_1987.find(2)->display, Display::masked_pan);
    EXPECT_EQ(fields_1987.find(52)->display, Display::redacted);
    EXPECT_EQ(fields_1987.find(128)->display, Display::clear);
}

TEST(FieldTable, FindsNothingForUndefinedFields) {
    for (const int number : {-1, 0, 1, 5, 64, 65, 127, 129}) {
        EXPECT_EQ(fields_1987.find(number), nullptr) << "field " << number;
    }
}

// Dereferencing a lookup is how these show it found something: a null
// dereference cannot be a constant expression, and GCC's UBSan build does not
// accept `pointer != nullptr` in a static_assert.
TEST(FieldTable, WorksInConstantExpressions) {
    static_assert(fields_1987.find(4)->max_length == 12);
    static_assert(fields_1987.find(5) == nullptr);
    constexpr const FieldSpec* additional_data = test::extended_table.find(48);
    static_assert(additional_data->length_type == LengthType::lllvar);
    EXPECT_EQ(additional_data->name, "Additional data, private");
}

TEST(FieldSpec, NotationForVariableLengths) {
    EXPECT_EQ(notation(*test::extended_table.find(48)), "ans...999");
    EXPECT_EQ(notation(*test::extended_table.find(55)), "b...255");
}

TEST(FieldSpec, PrefixDigitsFollowTheLengthType) {
    static_assert(prefix_digits(LengthType::fixed) == 0);
    EXPECT_EQ(prefix_digits(LengthType::llvar), 2U);
    EXPECT_EQ(prefix_digits(LengthType::lllvar), 3U);
}

TEST(FieldTableValidity, AcceptsOrderedFieldsWithLengthsTheirPrefixCanCarry) {
    EXPECT_TRUE(valid({}));
    EXPECT_TRUE(valid({fixed_n(2), fixed_n(64), fixed_n(66), fixed_n(128)}));
    EXPECT_TRUE(valid({{2, Format::n, LengthType::llvar, 99, Display::clear, "t"},
                       {3, Format::ans, LengthType::lllvar, 999, Display::clear, "t"}}));
    EXPECT_TRUE(is_valid_table(fields_1987_specs));
}

TEST(FieldTableValidity, RejectsBadNumbering) {
    EXPECT_FALSE(valid({fixed_n(3), fixed_n(2)})); // out of order
    EXPECT_FALSE(valid({fixed_n(3), fixed_n(3)})); // duplicate
    EXPECT_FALSE(valid({fixed_n(1)}));             // primary bitmap indicator
    EXPECT_FALSE(valid({fixed_n(65)}));            // tertiary bitmap indicator
    EXPECT_FALSE(valid({fixed_n(129)}));           // beyond the secondary bitmap
}

TEST(FieldTableValidity, RejectsLengthsThatCannotWork) {
    EXPECT_FALSE(valid({fixed_n(3, 0)}));
    EXPECT_FALSE(valid({{2, Format::n, LengthType::llvar, 100, Display::clear, "t"}}));
    EXPECT_FALSE(valid({{2, Format::n, LengthType::lllvar, 1000, Display::clear, "t"}}));
}

} // namespace
} // namespace iso8583

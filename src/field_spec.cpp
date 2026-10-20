#include "iso8583/field_spec.hpp"

#include <array>

#include "iso8583/fields_1987.hpp"

namespace iso8583 {

static_assert(is_valid_table(fields_1987_specs));

std::string notation(const FieldSpec& spec) {
    static constexpr std::array<std::string_view, 4> formats{"n", "an", "ans", "b"};
    static constexpr std::array<std::string_view, 3> separators{"", "..", "..."};

    std::string text{formats.at(static_cast<std::size_t>(spec.format))};
    text += separators.at(static_cast<std::size_t>(spec.length_type));
    text += std::to_string(spec.max_length);
    return text;
}

} // namespace iso8583

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace iso8583 {

// Data element representation, named as the standard writes it.
enum class Format : std::uint8_t {
    n,   // digits, carried as ASCII
    an,  // letters and digits (and space, which pads fixed-width values)
    ans, // printable ASCII
    b,   // raw bytes
};

enum class LengthType : std::uint8_t {
    fixed,
    llvar,  // two ASCII length digits, then the value
    lllvar, // three ASCII length digits, then the value
};

// How a field may appear when a message is printed or logged.
enum class Display : std::uint8_t {
    clear,
    masked_pan, // first six and last four digits only
    redacted,   // never shown
};

struct FieldSpec {
    int number = 0;
    Format format = Format::ans;
    LengthType length_type = LengthType::fixed;
    std::size_t max_length = 0; // characters, or bytes for Format::b
    Display display = Display::clear;
    std::string_view name;
};

[[nodiscard]] constexpr std::size_t prefix_digits(LengthType type) noexcept {
    return type == LengthType::llvar ? 2 : type == LengthType::lllvar ? 3 : 0;
}

// The standard's notation for a spec: "n6", "n..19", "ans...999", "b8".
[[nodiscard]] std::string notation(const FieldSpec& spec);

// What the codec relies on: field numbers strictly ascending within 2..128
// (1 and 65 are bitmap indicators, not data), every length positive, and every
// variable length small enough for its prefix to express.
[[nodiscard]] constexpr bool is_valid_table(std::span<const FieldSpec> specs) noexcept {
    int previous = 1;
    for (const FieldSpec& spec : specs) {
        const bool number_ok = spec.number > previous && spec.number <= 128 && spec.number != 65;
        const std::size_t prefix_limit = spec.length_type == LengthType::llvar ? 99 : 999;
        const bool length_ok = spec.max_length > 0 && (spec.length_type == LengthType::fixed ||
                                                       spec.max_length <= prefix_limit);
        if (!number_ok || !length_ok) {
            return false;
        }
        previous = spec.number;
    }
    return true;
}

namespace detail {
// Never defined and deliberately not constexpr; see mti.hpp for the idiom.
void field_table_is_invalid();
} // namespace detail

// A dialect's field definitions. Tables are fixed when the program is built,
// so the constructor is consteval and an invalid table is a compile error.
// The table only views the specs, which must outlive it (in practice they are
// a constexpr array at namespace scope).
class FieldTable {
public:
    consteval explicit FieldTable(std::span<const FieldSpec> specs) : specs_{specs} {
        if (!is_valid_table(specs)) {
            detail::field_table_is_invalid();
        }
    }

    // The spec for a field, or nullptr when this table does not define it.
    [[nodiscard]] constexpr const FieldSpec* find(int number) const noexcept {
        const auto it = std::ranges::lower_bound(specs_, number, {}, &FieldSpec::number);
        return it != specs_.end() && it->number == number ? &*it : nullptr;
    }

    [[nodiscard]] constexpr std::span<const FieldSpec> specs() const noexcept { return specs_; }

private:
    std::span<const FieldSpec> specs_;
};

} // namespace iso8583

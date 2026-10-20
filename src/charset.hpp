#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>

#include "iso8583/field_spec.hpp"

// Character classes for the text formats. Deliberately not <cctype>: these
// must not depend on the locale, and char may be signed.
namespace iso8583::charset {

[[nodiscard]] constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

[[nodiscard]] constexpr bool is_alpha(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

[[nodiscard]] constexpr bool is_printable(char c) noexcept {
    return c >= ' ' && c <= '~';
}

// Space is accepted in an fields because fixed-width an values are space
// padded on the wire.
[[nodiscard]] constexpr bool allowed(Format format, char c) noexcept {
    if (format == Format::n) {
        return is_digit(c);
    }
    if (format == Format::an) {
        return is_digit(c) || is_alpha(c) || c == ' ';
    }
    return is_printable(c);
}

// Index of the first character the format does not allow, if any.
[[nodiscard]] constexpr std::optional<std::size_t>
first_disallowed(Format format, std::string_view text) noexcept {
    const auto it = std::ranges::find_if_not(text, [format](char c) { return allowed(format, c); });
    if (it == text.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - text.begin());
}

} // namespace iso8583::charset

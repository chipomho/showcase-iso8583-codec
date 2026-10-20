#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "iso8583/result.hpp"

namespace iso8583 {

namespace detail {
// Never defined and deliberately not constexpr: reaching a call to it during
// constant evaluation is what turns a bad literal into a compile error, and
// the compiler's message names it.
void mti_literal_must_be_four_digits();
} // namespace detail

// Message type indicator: four digits giving the ISO 8583 version, message
// class, function and origin. 0200 is a 1987 financial request; 0210 is the
// response to it.
class Mti {
public:
    // For literals, checked at compile time: Mti{"02X0"} does not build.
    consteval explicit Mti(const char (&digits)[5]) {
        for (std::size_t i = 0; i < digits_.size(); ++i) {
            if (digits[i] < '0' || digits[i] > '9') {
                detail::mti_literal_must_be_four_digits();
            }
            digits_[i] = digits[i];
        }
    }

    // For text that arrives at run time.
    [[nodiscard]] static Result<Mti> parse(std::string_view text);

    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return {digits_.data(), digits_.size()};
    }

    [[nodiscard]] constexpr int version() const noexcept { return digit(0); }
    [[nodiscard]] constexpr int message_class() const noexcept { return digit(1); }
    [[nodiscard]] constexpr int function() const noexcept { return digit(2); }
    [[nodiscard]] constexpr int origin() const noexcept { return digit(3); }

    friend constexpr bool operator==(const Mti&, const Mti&) = default;

private:
    constexpr Mti() = default;

    [[nodiscard]] constexpr int digit(std::size_t index) const noexcept {
        return digits_[index] - '0';
    }

    std::array<char, 4> digits_{};
};

// Plain-English reading, e.g. "financial request (origin acquirer, ISO 8583:1987)".
[[nodiscard]] std::string describe(const Mti& mti);

} // namespace iso8583

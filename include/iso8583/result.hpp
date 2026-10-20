#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace iso8583 {

// Every way an encode or decode can fail. Malformed input is reported with one
// of these; it is never an exception.
enum class Errc : std::uint8_t {
    truncated,              // the input ended part way through an element
    bad_mti,                // the MTI is not four ASCII digits
    bad_length_prefix,      // an LL or LLL prefix is not ASCII digits
    length_exceeds_maximum, // a value is longer than its field allows
    wrong_length,           // a fixed-length binary value is not exactly the field's size
    non_numeric,            // an n field holds something other than digits
    invalid_character,      // an an or ans field holds a character outside its set
    unknown_field,          // the field is not in the field table
    type_mismatch,          // text supplied for a binary field, or bytes for a text field
    trailing_data,          // bytes remain after the last field
};

[[nodiscard]] std::string_view describe(Errc code) noexcept;

struct Error {
    constexpr Error(Errc error_code, int field_number = 0,
                    std::optional<std::size_t> byte_offset = std::nullopt) noexcept
        : code{error_code}, field{field_number}, offset{byte_offset} {}

    Errc code;
    int field;                         // data element number, 0 when not about one field
    std::optional<std::size_t> offset; // byte offset into the input; decode errors only

    friend bool operator==(const Error&, const Error&) = default;
};

// For example "field 4: numeric field contains a non-digit (offset 31)".
[[nodiscard]] std::string to_string(const Error& error);

// A value, or the Error explaining why there is none. C++20 has no
// std::expected and this is all the codec needs from one.
template <typename T>
class [[nodiscard]] Result {
public:
    Result(T value) : state_{std::in_place_index<0>, std::move(value)} {}
    Result(Error error) : state_{std::in_place_index<1>, std::move(error)} {}

    [[nodiscard]] bool has_value() const noexcept { return state_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }

    // Asking for the value of an error (or the error of a value) is a
    // programming mistake and throws std::bad_variant_access, as std::get does.
    [[nodiscard]] T& value() & { return std::get<0>(state_); }
    [[nodiscard]] const T& value() const& { return std::get<0>(state_); }
    [[nodiscard]] T&& value() && { return std::get<0>(std::move(state_)); }
    [[nodiscard]] const Error& error() const { return std::get<1>(state_); }

    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T& operator*() const& { return value(); }
    [[nodiscard]] T* operator->() { return &value(); }
    [[nodiscard]] const T* operator->() const { return &value(); }

private:
    std::variant<T, Error> state_;
};

} // namespace iso8583

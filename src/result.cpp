#include "iso8583/result.hpp"

namespace iso8583 {

std::string_view describe(Errc code) noexcept {
    switch (code) {
    case Errc::truncated:
        return "input ended part way through an element";
    case Errc::bad_mti:
        return "message type indicator is not four digits";
    case Errc::bad_length_prefix:
        return "length prefix is not numeric";
    case Errc::length_exceeds_maximum:
        return "value is longer than the field allows";
    case Errc::wrong_length:
        return "binary value is not the field's fixed length";
    case Errc::non_numeric:
        return "numeric field contains a non-digit";
    case Errc::invalid_character:
        return "field contains a character outside its character set";
    case Errc::unknown_field:
        return "field is not in the field table";
    case Errc::type_mismatch:
        return "value type does not match the field format";
    case Errc::trailing_data:
        return "bytes remain after the last field";
    }
    return "unrecognised error code";
}

std::string to_string(const Error& error) {
    std::string text;
    if (error.field != 0) {
        text += "field ";
        text += std::to_string(error.field);
        text += ": ";
    }
    text += describe(error.code);
    if (error.offset) {
        text += " (offset ";
        text += std::to_string(*error.offset);
        text += ')';
    }
    return text;
}

} // namespace iso8583

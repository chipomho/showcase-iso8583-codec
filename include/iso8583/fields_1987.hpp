#pragma once

#include <array>

#include "iso8583/field_spec.hpp"

namespace iso8583 {

// The subset of ISO 8583:1987 a basic card authorisation and financial
// exchange needs. Other dialects (a scheme's or a switch's) are just another
// table built the same way.
inline constexpr std::array<FieldSpec, 15> fields_1987_specs{{
    {2, Format::n, LengthType::llvar, 19, Display::masked_pan, "Primary account number"},
    {3, Format::n, LengthType::fixed, 6, Display::clear, "Processing code"},
    {4, Format::n, LengthType::fixed, 12, Display::clear, "Amount, transaction"},
    {7, Format::n, LengthType::fixed, 10, Display::clear, "Transmission date and time"},
    {11, Format::n, LengthType::fixed, 6, Display::clear, "Systems trace audit number"},
    {12, Format::n, LengthType::fixed, 6, Display::clear, "Time, local transaction"},
    {13, Format::n, LengthType::fixed, 4, Display::clear, "Date, local transaction"},
    {37, Format::an, LengthType::fixed, 12, Display::clear, "Retrieval reference number"},
    {38, Format::an, LengthType::fixed, 6, Display::clear, "Authorization identification response"},
    {39, Format::an, LengthType::fixed, 2, Display::clear, "Response code"},
    {41, Format::ans, LengthType::fixed, 8, Display::clear,
     "Card acceptor terminal identification"},
    {42, Format::ans, LengthType::fixed, 15, Display::clear, "Card acceptor identification code"},
    {49, Format::n, LengthType::fixed, 3, Display::clear, "Currency code, transaction"},
    {52, Format::b, LengthType::fixed, 8, Display::redacted, "Personal identification number data"},
    {128, Format::b, LengthType::fixed, 8, Display::clear, "Message authentication code"},
}};

inline constexpr FieldTable fields_1987{fields_1987_specs};

} // namespace iso8583

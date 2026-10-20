#pragma once

// Known messages and their wire bytes. The bytes are assembled by hand from
// the layout rules, not produced by the encoder, so they are an independent
// check on it.

#include <array>

#include "iso8583/field_spec.hpp"
#include "iso8583/message.hpp"
#include "test_support.hpp"

namespace iso8583::test {

inline Bytes pin_block() {
    return hex("9F3A510CE277841B");
}
inline Bytes mac() {
    return hex("5B8E21C407F93D62");
}

// 0200 purchase with online PIN and a MAC, so it needs the secondary bitmap.
inline Message financial_request() {
    Message message{Mti{"0200"}};
    message.set(2, "4111111111111111")
        .set(3, "000000")
        .set(4, "000000002500")
        .set(7, "1020214500")
        .set(11, "000123")
        .set(12, "084500")
        .set(13, "1021")
        .set(37, "629400000123")
        .set(41, "TERM0001")
        .set(42, "MERCHANT0000042")
        .set(49, "036")
        .set(52, pin_block())
        .set(128, mac());
    return message;
}

inline Bytes financial_request_wire() {
    return join({
        ascii("0200"),
        // Primary: fields 1 (secondary follows), 2, 3, 4, 7, 11, 12, 13, 37, 41, 42, 49, 52.
        hex("F238000008C09000"),
        // Secondary: field 128.
        hex("0000000000000001"),
        ascii("16"),
        ascii("4111111111111111"),
        ascii("000000"),
        ascii("000000002500"),
        ascii("1020214500"),
        ascii("000123"),
        ascii("084500"),
        ascii("1021"),
        ascii("629400000123"),
        ascii("TERM0001"),
        ascii("MERCHANT0000042"),
        ascii("036"),
        pin_block(),
        mac(),
    });
}

// The approval for it. No field above 64, so only the primary bitmap.
inline Message financial_response() {
    Message message{Mti{"0210"}};
    message.set(2, "4111111111111111")
        .set(3, "000000")
        .set(4, "000000002500")
        .set(7, "1020214501")
        .set(11, "000123")
        .set(37, "629400000123")
        .set(38, "A1B2C3")
        .set(39, "00")
        .set(41, "TERM0001")
        .set(49, "036");
    return message;
}

inline Bytes financial_response_wire() {
    return join({
        ascii("0210"),
        // Fields 2, 3, 4, 7, 11, 37, 38, 39, 41, 49.
        hex("722000000E808000"),
        ascii("164111111111111111"),
        ascii("000000"),
        ascii("000000002500"),
        ascii("1020214501"),
        ascii("000123"),
        ascii("629400000123"),
        ascii("A1B2C3"),
        ascii("00"),
        ascii("TERM0001"),
        ascii("036"),
    });
}

// A dialect with the shapes the 1987 subset does not use: LLLVAR text, and
// LLLVAR binary as EMV chip data in field 55 is commonly carried.
inline constexpr std::array<FieldSpec, 3> extended_specs{{
    {2, Format::n, LengthType::llvar, 19, Display::masked_pan, "Primary account number"},
    {48, Format::ans, LengthType::lllvar, 999, Display::clear, "Additional data, private"},
    {55, Format::b, LengthType::lllvar, 255, Display::clear, "ICC system related data"},
}};
inline constexpr FieldTable extended_table{extended_specs};

} // namespace iso8583::test

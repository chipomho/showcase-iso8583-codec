#pragma once

#include <cstddef>
#include <span>

#include "iso8583/bitmap.hpp"
#include "iso8583/bytes.hpp"
#include "iso8583/field_spec.hpp"
#include "iso8583/fields_1987.hpp"
#include "iso8583/message.hpp"
#include "iso8583/result.hpp"

namespace iso8583 {

// Wire layout: the four MTI digits in ASCII, the 8-byte binary primary bitmap,
// the 8-byte secondary bitmap when any field from 65 to 128 is present, then
// each present field in ascending order. n, an and ans values and the LL/LLL
// length prefixes are ASCII; b values are raw bytes.

// Serialises a message. Fails, rather than writing something a host would
// reject, when a field is not in the table, holds the wrong type, is too long,
// or contains characters its format does not allow. Short fixed-length text is
// padded the way hosts expect: n with leading zeros, an and ans with trailing
// spaces. Fixed-length binary values must be exactly the right size.
[[nodiscard]] Result<Bytes> encode(const Message& message, const FieldTable& table = fields_1987);

// Parses one complete message. Never throws on malformed input: the Error says
// what was wrong, in which field, and at which byte offset. Bytes left over
// after the last field are an error, since they usually mean a framing fault.
[[nodiscard]] Result<Message> decode(std::span<const std::byte> input,
                                     const FieldTable& table = fields_1987);

// The bitmap encode() writes for this message's fields.
[[nodiscard]] Bitmap bitmap_of(const Message& message);

} // namespace iso8583

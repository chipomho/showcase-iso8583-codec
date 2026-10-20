#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

#include "iso8583/field_spec.hpp"
#include "iso8583/fields_1987.hpp"
#include "iso8583/message.hpp"

namespace iso8583 {

// Keeps the first six and last four digits and replaces the rest with '*'.
// A PAN too short to hide anything that way is masked completely.
[[nodiscard]] std::string mask_pan(std::string_view pan);

// Writes a readable dump of the message, one field per line. Each value is
// shown according to its spec's Display rule, so the PAN is always masked and
// the PIN block never appears. Fields the table does not know are redacted,
// because nothing says they are safe to show.
void print(std::ostream& out, const Message& message, const FieldTable& table = fields_1987);

// Same as print() with the 1987 table.
std::ostream& operator<<(std::ostream& out, const Message& message);

} // namespace iso8583

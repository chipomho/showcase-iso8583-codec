#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iso8583 {

using Bytes = std::vector<std::byte>;

// Upper-case hex with no separators: {0x30, 0xF2} gives "30F2".
[[nodiscard]] std::string to_hex(std::span<const std::byte> bytes);

// Parses hex in either case. ASCII whitespace is skipped, so a message copied
// from a trace as "30 32 30 30" works. Returns nullopt for a non-hex character
// or an odd number of digits.
[[nodiscard]] std::optional<Bytes> from_hex(std::string_view text);

} // namespace iso8583

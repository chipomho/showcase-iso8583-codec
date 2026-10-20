#include "iso8583/bytes.hpp"

namespace iso8583 {
namespace {

constexpr int hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

constexpr bool is_space(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

} // namespace

std::string to_hex(std::span<const std::byte> bytes) {
    constexpr std::string_view digits = "0123456789ABCDEF";
    std::string text;
    text.reserve(bytes.size() * 2);
    for (const std::byte byte : bytes) {
        const auto value = std::to_integer<unsigned>(byte);
        text += digits[value >> 4U];
        text += digits[value & 0x0FU];
    }
    return text;
}

std::optional<Bytes> from_hex(std::string_view text) {
    Bytes bytes;
    bytes.reserve(text.size() / 2);
    int high = -1;
    for (const char c : text) {
        if (is_space(c)) {
            continue;
        }
        const int nibble = hex_value(c);
        if (nibble < 0) {
            return std::nullopt;
        }
        if (high < 0) {
            high = nibble;
        } else {
            bytes.push_back(static_cast<std::byte>((high << 4) | nibble));
            high = -1;
        }
    }
    if (high >= 0) {
        return std::nullopt;
    }
    return bytes;
}

} // namespace iso8583

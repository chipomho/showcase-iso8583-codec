#pragma once

#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <string_view>

#include "iso8583/bytes.hpp"
#include "iso8583/result.hpp"

namespace iso8583 {

// Found by GoogleTest through ADL, so failed expectations print readable errors.
inline void PrintTo(const Error& error, std::ostream* os) {
    *os << to_string(error);
}
inline void PrintTo(Errc code, std::ostream* os) {
    *os << describe(code);
}

} // namespace iso8583

namespace iso8583::test {

// Wire bytes for ASCII text, e.g. a length prefix and value.
inline Bytes ascii(std::string_view text) {
    Bytes bytes;
    for (const char c : text) {
        bytes.push_back(static_cast<std::byte>(c));
    }
    return bytes;
}

inline Bytes hex(std::string_view text) {
    auto bytes = from_hex(text);
    if (!bytes) {
        throw std::invalid_argument("bad hex in test data");
    }
    return *bytes;
}

inline Bytes join(std::initializer_list<Bytes> parts) {
    Bytes joined;
    for (const Bytes& part : parts) {
        joined.insert(joined.end(), part.begin(), part.end());
    }
    return joined;
}

} // namespace iso8583::test

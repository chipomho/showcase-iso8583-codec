#include "iso8583/mti.hpp"

#include <algorithm>

#include "charset.hpp"

namespace iso8583 {

Result<Mti> Mti::parse(std::string_view text) {
    if (text.size() != 4 || !std::ranges::all_of(text, charset::is_digit)) {
        return Error{Errc::bad_mti};
    }
    Mti mti;
    std::ranges::copy(text, mti.digits_.begin());
    return mti;
}

std::string describe(const Mti& mti) {
    static constexpr std::array<std::string_view, 10> versions{
        "ISO 8583:1987",    "ISO 8583:1993",    "ISO 8583:2003",    "reserved version",
        "reserved version", "reserved version", "reserved version", "reserved version",
        "national use",     "private use"};
    static constexpr std::array<std::string_view, 10> classes{
        "reserved class",         "authorization",  "financial",      "file action",
        "reversal or chargeback", "reconciliation", "administrative", "fee collection",
        "network management",     "reserved class"};
    static constexpr std::array<std::string_view, 10> functions{"request",
                                                                "request response",
                                                                "advice",
                                                                "advice response",
                                                                "notification",
                                                                "notification acknowledgement",
                                                                "instruction",
                                                                "instruction acknowledgement",
                                                                "reserved function",
                                                                "reserved function"};
    static constexpr std::array<std::string_view, 10> origins{
        "acquirer",     "acquirer repeat", "issuer",   "issuer repeat", "other",
        "other repeat", "reserved",        "reserved", "reserved",      "reserved"};

    // Every digit is 0-9 by construction, so indexing cannot go out of range.
    const auto at = [](const auto& table, int digit) {
        return table[static_cast<std::size_t>(digit)];
    };

    std::string text;
    text += at(classes, mti.message_class());
    text += ' ';
    text += at(functions, mti.function());
    text += " (origin ";
    text += at(origins, mti.origin());
    text += ", ";
    text += at(versions, mti.version());
    text += ')';
    return text;
}

} // namespace iso8583

#include "iso8583/print.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>

#include "iso8583/bytes.hpp"
#include "iso8583/codec.hpp"

namespace iso8583 {
namespace {

constexpr std::size_t pan_keep_front = 6;
constexpr std::size_t pan_keep_back = 4;
constexpr std::string_view redacted = "[redacted]";

std::string shown_value(const FieldSpec* spec, const FieldValue& value) {
    if (spec == nullptr || spec->display == Display::redacted) {
        return std::string{redacted};
    }
    if (const auto* bytes = std::get_if<Bytes>(&value)) {
        // A PAN field holding bytes is malformed; don't guess how to mask it.
        if (spec->display != Display::clear) {
            return std::string{redacted};
        }
        return to_hex(*bytes);
    }
    const auto& text = std::get<std::string>(value);
    if (spec->display == Display::masked_pan) {
        return mask_pan(text);
    }
    return text;
}

} // namespace

std::string mask_pan(std::string_view pan) {
    std::string masked(pan.size(), '*');
    if (pan.size() > pan_keep_front + pan_keep_back) {
        masked.replace(0, pan_keep_front, pan.substr(0, pan_keep_front));
        masked.replace(pan.size() - pan_keep_back, pan_keep_back,
                       pan.substr(pan.size() - pan_keep_back));
    }
    return masked;
}

void print(std::ostream& out, const Message& message, const FieldTable& table) {
    // Format into a local stream so the caller's stream flags are left alone.
    std::ostringstream text;
    text << message.mti().view() << "  " << describe(message.mti()) << '\n';

    Bytes bitmap;
    bitmap_of(message).append_to(bitmap);
    const std::span<const std::byte> bitmap_bytes{bitmap};
    text << "bitmap " << to_hex(bitmap_bytes.first(8));
    if (bitmap_bytes.size() > 8) {
        text << ' ' << to_hex(bitmap_bytes.subspan(8));
    }
    text << '\n';

    for (const auto& [number, value] : message.fields()) {
        const FieldSpec* spec = table.find(number);
        const std::string format = spec != nullptr ? notation(*spec) : std::string{"?"};
        const std::string_view name = spec != nullptr ? spec->name : "(not in field table)";

        text << "  " << std::setfill('0') << std::right << std::setw(3) << number;
        text << std::setfill(' ') << std::left << "  " << std::setw(8) << format;
        text << std::setw(40) << name << shown_value(spec, value) << '\n';
    }
    out << text.str();
}

std::ostream& operator<<(std::ostream& out, const Message& message) {
    print(out, message);
    return out;
}

} // namespace iso8583

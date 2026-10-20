#include "iso8583/codec.hpp"

#include <optional>
#include <string>
#include <string_view>

#include "charset.hpp"

namespace iso8583 {
namespace {

constexpr std::size_t mti_length = 4;

// std::byte and char may alias, so viewing received bytes as text is legal and
// avoids a copy.
std::string_view as_chars(std::span<const std::byte> bytes) noexcept {
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void append_ascii(Bytes& out, std::string_view text) {
    for (const char c : text) {
        out.push_back(static_cast<std::byte>(c));
    }
}

void append_length_prefix(Bytes& out, std::size_t length, std::size_t digits) {
    std::string prefix(digits, '0');
    for (auto it = prefix.rbegin(); it != prefix.rend(); ++it) {
        *it = static_cast<char>('0' + length % 10);
        length /= 10;
    }
    append_ascii(out, prefix);
}

std::size_t parse_decimal(std::string_view digits) noexcept {
    std::size_t value = 0;
    for (const char c : digits) {
        value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    return value;
}

Errc charset_error(Format format) noexcept {
    return format == Format::n ? Errc::non_numeric : Errc::invalid_character;
}

std::optional<Error> encode_binary(const FieldSpec& spec, const FieldValue& value, Bytes& out) {
    const auto* bytes = std::get_if<Bytes>(&value);
    if (bytes == nullptr) {
        return Error{Errc::type_mismatch, spec.number};
    }
    if (spec.length_type == LengthType::fixed && bytes->size() != spec.max_length) {
        return Error{Errc::wrong_length, spec.number};
    }
    if (bytes->size() > spec.max_length) {
        return Error{Errc::length_exceeds_maximum, spec.number};
    }
    append_length_prefix(out, bytes->size(), prefix_digits(spec.length_type));
    out.insert(out.end(), bytes->begin(), bytes->end());
    return std::nullopt;
}

std::optional<Error> encode_text(const FieldSpec& spec, const FieldValue& value, Bytes& out) {
    const auto* text = std::get_if<std::string>(&value);
    if (text == nullptr) {
        return Error{Errc::type_mismatch, spec.number};
    }
    if (text->size() > spec.max_length) {
        return Error{Errc::length_exceeds_maximum, spec.number};
    }
    if (charset::first_disallowed(spec.format, *text)) {
        return Error{charset_error(spec.format), spec.number};
    }
    if (spec.length_type != LengthType::fixed) {
        append_length_prefix(out, text->size(), prefix_digits(spec.length_type));
        append_ascii(out, *text);
        return std::nullopt;
    }
    const std::string padding(spec.max_length - text->size(), spec.format == Format::n ? '0' : ' ');
    if (spec.format == Format::n) {
        append_ascii(out, padding);
        append_ascii(out, *text);
    } else {
        append_ascii(out, *text);
        append_ascii(out, padding);
    }
    return std::nullopt;
}

// Reads one field starting at `offset`, which on success is advanced past it.
std::optional<Error> decode_field(const FieldSpec& spec, std::span<const std::byte> input,
                                  std::size_t& offset, Message& message) {
    const std::size_t start = offset;
    std::size_t length = spec.max_length;

    if (const std::size_t digits = prefix_digits(spec.length_type); digits > 0) {
        if (input.size() - offset < digits) {
            return Error{Errc::truncated, spec.number, start};
        }
        const std::string_view prefix = as_chars(input.subspan(offset, digits));
        if (charset::first_disallowed(Format::n, prefix)) {
            return Error{Errc::bad_length_prefix, spec.number, start};
        }
        length = parse_decimal(prefix);
        if (length > spec.max_length) {
            return Error{Errc::length_exceeds_maximum, spec.number, start};
        }
        offset += digits;
    }

    if (input.size() - offset < length) {
        return Error{Errc::truncated, spec.number, start};
    }
    const auto raw = input.subspan(offset, length);

    if (spec.format == Format::b) {
        message.set(spec.number, Bytes(raw.begin(), raw.end()));
    } else {
        const std::string_view text = as_chars(raw);
        if (const auto bad = charset::first_disallowed(spec.format, text)) {
            return Error{charset_error(spec.format), spec.number, offset + *bad};
        }
        message.set(spec.number, std::string{text});
    }
    offset += length;
    return std::nullopt;
}

} // namespace

Bitmap bitmap_of(const Message& message) {
    Bitmap bitmap;
    for (const auto& entry : message.fields()) {
        bitmap.set(entry.first);
    }
    return bitmap;
}

Result<Bytes> encode(const Message& message, const FieldTable& table) {
    for (const auto& entry : message.fields()) {
        if (table.find(entry.first) == nullptr) {
            return Error{Errc::unknown_field, entry.first};
        }
    }

    Bytes out;
    append_ascii(out, message.mti().view());
    bitmap_of(message).append_to(out);

    for (const auto& [number, value] : message.fields()) {
        const FieldSpec& spec = *table.find(number);
        const auto error = spec.format == Format::b ? encode_binary(spec, value, out)
                                                    : encode_text(spec, value, out);
        if (error) {
            return *error;
        }
    }
    return out;
}

Result<Message> decode(std::span<const std::byte> input, const FieldTable& table) {
    if (input.size() < mti_length) {
        return Error{Errc::truncated, 0, 0};
    }
    const auto mti = Mti::parse(as_chars(input.first(mti_length)));
    if (!mti) {
        return Error{Errc::bad_mti, 0, 0};
    }

    const auto bitmap = Bitmap::read(input.subspan(mti_length));
    if (!bitmap) {
        return Error{bitmap.error().code, 0, mti_length + bitmap.error().offset.value_or(0)};
    }

    Message message{*mti};
    std::size_t offset = mti_length + bitmap->size_bytes();
    for (int number = 2; number <= Bitmap::max_field; ++number) {
        if (!bitmap->test(number)) {
            continue;
        }
        const FieldSpec* spec = table.find(number);
        if (spec == nullptr) {
            return Error{Errc::unknown_field, number, offset};
        }
        if (const auto error = decode_field(*spec, input, offset, message)) {
            return *error;
        }
    }

    if (offset != input.size()) {
        return Error{Errc::trailing_data, 0, offset};
    }
    return message;
}

} // namespace iso8583

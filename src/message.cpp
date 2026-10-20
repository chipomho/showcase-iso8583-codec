#include "iso8583/message.hpp"

#include <utility>

namespace iso8583 {

Message& Message::set(int field, std::string value) {
    fields_.insert_or_assign(field, std::move(value));
    return *this;
}

Message& Message::set(int field, Bytes value) {
    fields_.insert_or_assign(field, std::move(value));
    return *this;
}

bool Message::erase(int field) {
    return fields_.erase(field) > 0;
}

bool Message::has(int field) const {
    return fields_.contains(field);
}

const FieldValue* Message::find(int field) const {
    const auto it = fields_.find(field);
    return it == fields_.end() ? nullptr : &it->second;
}

std::optional<std::string_view> Message::text(int field) const {
    const auto* value = find(field);
    const auto* text = value == nullptr ? nullptr : std::get_if<std::string>(value);
    if (text == nullptr) {
        return std::nullopt;
    }
    return std::string_view{*text};
}

std::optional<std::span<const std::byte>> Message::binary(int field) const {
    const auto* value = find(field);
    const auto* bytes = value == nullptr ? nullptr : std::get_if<Bytes>(value);
    if (bytes == nullptr) {
        return std::nullopt;
    }
    return std::span<const std::byte>{*bytes};
}

} // namespace iso8583

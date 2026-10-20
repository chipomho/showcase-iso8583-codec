#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include "iso8583/bytes.hpp"
#include "iso8583/mti.hpp"

namespace iso8583 {

// n, an and ans values are text; b values are bytes.
using FieldValue = std::variant<std::string, Bytes>;

// An MTI plus its data elements, keyed by field number. This is a plain value
// type: nothing is checked when a field is set. encode() validates every field
// against a FieldTable, and decode() only ever produces valid messages.
class Message {
public:
    explicit Message(Mti mti) noexcept : mti_{mti} {}

    [[nodiscard]] const Mti& mti() const noexcept { return mti_; }

    // Adds or replaces a field. Returns *this so a message can be built in one
    // expression.
    Message& set(int field, std::string value);
    Message& set(int field, Bytes value);

    // Removes a field; returns whether it was present.
    bool erase(int field);

    [[nodiscard]] bool has(int field) const;

    // The field's value, or nullptr when it is absent.
    [[nodiscard]] const FieldValue* find(int field) const;

    // Typed access. Empty when the field is absent or holds the other type.
    // The views stay valid until the field is changed or the message destroyed.
    [[nodiscard]] std::optional<std::string_view> text(int field) const;
    [[nodiscard]] std::optional<std::span<const std::byte>> binary(int field) const;

    // In ascending field number, which is wire order.
    [[nodiscard]] const std::map<int, FieldValue>& fields() const noexcept { return fields_; }

    friend bool operator==(const Message&, const Message&) = default;

private:
    Mti mti_;
    std::map<int, FieldValue> fields_;
};

} // namespace iso8583

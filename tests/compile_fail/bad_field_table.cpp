// Must not compile: a field table that is not in field number order.
#include <array>

#include "iso8583/field_spec.hpp"

namespace {

using iso8583::Display;
using iso8583::FieldSpec;
using iso8583::Format;
using iso8583::LengthType;

constexpr std::array<FieldSpec, 2> out_of_order{{
    {4, Format::n, LengthType::fixed, 12, Display::clear, "Amount, transaction"},
    {3, Format::n, LengthType::fixed, 6, Display::clear, "Processing code"},
}};

} // namespace

int main() {
    constexpr iso8583::FieldTable table{out_of_order};
    return table.find(3) == nullptr ? 1 : 0;
}

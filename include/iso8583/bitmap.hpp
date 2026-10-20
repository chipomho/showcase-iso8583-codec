#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "iso8583/result.hpp"

namespace iso8583 {

// The primary and secondary bitmaps. Bit n (counting from 1 at the most
// significant bit of the first byte) says whether data element n is present.
// Bit 1 says a secondary bitmap, covering fields 65 to 128, follows the primary.
class Bitmap {
public:
    static constexpr int max_field = 128;

    // Setting any field from 65 up also sets bit 1, so the bitmap can never
    // claim a secondary field without announcing the secondary bitmap.
    // Numbers outside 1..128 are ignored.
    constexpr void set(int field) noexcept {
        if (!in_range(field)) {
            return;
        }
        words_[word_index(field)] |= mask(field);
        if (field > 64) {
            words_[0] |= mask(1);
        }
    }

    [[nodiscard]] constexpr bool test(int field) const noexcept {
        return in_range(field) && (words_[word_index(field)] & mask(field)) != 0;
    }

    [[nodiscard]] constexpr bool has_secondary() const noexcept { return test(1); }

    // 8 bytes on the wire, or 16 with the secondary bitmap.
    [[nodiscard]] constexpr std::size_t size_bytes() const noexcept {
        return has_secondary() ? 16 : 8;
    }

    void append_to(std::vector<std::byte>& out) const;

    // Reads the primary bitmap and, when bit 1 is set, the secondary. Error
    // offsets are relative to the start of `input`.
    [[nodiscard]] static Result<Bitmap> read(std::span<const std::byte> input);

    friend constexpr bool operator==(const Bitmap&, const Bitmap&) = default;

private:
    static constexpr bool in_range(int field) noexcept { return field >= 1 && field <= max_field; }

    static constexpr std::size_t word_index(int field) noexcept {
        return static_cast<std::size_t>(field - 1) / 64;
    }

    static constexpr std::uint64_t mask(int field) noexcept {
        return std::uint64_t{1} << (63 - (field - 1) % 64);
    }

    std::array<std::uint64_t, 2> words_{};
};

} // namespace iso8583

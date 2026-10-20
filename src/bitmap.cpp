#include "iso8583/bitmap.hpp"

namespace iso8583 {
namespace {

constexpr std::size_t word_bytes = 8;

std::uint64_t load_big_endian(std::span<const std::byte> bytes) noexcept {
    std::uint64_t value = 0;
    for (const std::byte byte : bytes) {
        value = (value << 8U) | std::to_integer<std::uint64_t>(byte);
    }
    return value;
}

} // namespace

void Bitmap::append_to(std::vector<std::byte>& out) const {
    const std::size_t words = has_secondary() ? 2 : 1;
    for (std::size_t w = 0; w < words; ++w) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::byte>((words_[w] >> shift) & 0xFFU));
        }
    }
}

Result<Bitmap> Bitmap::read(std::span<const std::byte> input) {
    if (input.size() < word_bytes) {
        return Error{Errc::truncated, 0, 0};
    }
    Bitmap bitmap;
    bitmap.words_[0] = load_big_endian(input.first(word_bytes));
    if (bitmap.has_secondary()) {
        if (input.size() < 2 * word_bytes) {
            return Error{Errc::truncated, 0, word_bytes};
        }
        bitmap.words_[1] = load_big_endian(input.subspan(word_bytes, word_bytes));
    }
    return bitmap;
}

} // namespace iso8583

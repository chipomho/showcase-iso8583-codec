#include <gtest/gtest.h>

#include <initializer_list>
#include <string>

#include "iso8583/bitmap.hpp"
#include "iso8583/bytes.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

using test::hex;

std::string wire_hex(const Bitmap& bitmap) {
    Bytes out;
    bitmap.append_to(out);
    return to_hex(out);
}

Bitmap with_fields(std::initializer_list<int> fields) {
    Bitmap bitmap;
    for (const int field : fields) {
        bitmap.set(field);
    }
    return bitmap;
}

TEST(Bitmap, EmptyBitmapIsEightZeroBytes) {
    const Bitmap bitmap;
    EXPECT_EQ(wire_hex(bitmap), "0000000000000000");
    EXPECT_EQ(bitmap.size_bytes(), 8U);
    EXPECT_FALSE(bitmap.has_secondary());
}

TEST(Bitmap, FieldOneIsTheMostSignificantBitOfTheFirstByte) {
    EXPECT_EQ(wire_hex(with_fields({2})), "4000000000000000");
    EXPECT_EQ(wire_hex(with_fields({8, 9})), "0180000000000000");
    EXPECT_EQ(wire_hex(with_fields({64})), "0000000000000001");
}

TEST(Bitmap, AnyFieldAbove64AnnouncesTheSecondaryBitmap) {
    const Bitmap bitmap = with_fields({65});
    EXPECT_TRUE(bitmap.has_secondary());
    EXPECT_EQ(bitmap.size_bytes(), 16U);
    EXPECT_EQ(wire_hex(bitmap), "8000000000000000"
                                "8000000000000000");
    EXPECT_EQ(wire_hex(with_fields({128})), "8000000000000000"
                                            "0000000000000001");
}

TEST(Bitmap, EqualityLooksAtBothBitmaps) {
    EXPECT_EQ(with_fields({70, 2}), with_fields({2, 70}));
    EXPECT_NE(with_fields({2}), with_fields({3}));
    EXPECT_NE(with_fields({2, 70}), with_fields({2, 71}));
}

TEST(Bitmap, IgnoresNumbersOutsideOneTo128) {
    const Bitmap bitmap = with_fields({-1, 0, 129, 1000});
    EXPECT_EQ(bitmap, Bitmap{});
    EXPECT_FALSE(bitmap.test(0));
    EXPECT_FALSE(bitmap.test(129));
}

TEST(Bitmap, WorksInConstantExpressions) {
    constexpr Bitmap bitmap = [] {
        Bitmap b;
        b.set(3);
        b.set(70);
        return b;
    }();
    static_assert(bitmap.test(3) && bitmap.test(70) && bitmap.has_secondary());
    static_assert(!bitmap.test(4));
    EXPECT_EQ(bitmap.size_bytes(), 16U);
}

TEST(Bitmap, ReadsWhatItWrites) {
    const Bitmap written = with_fields({2, 3, 4, 7, 11, 39, 64, 70, 100, 128});
    Bytes out;
    written.append_to(out);

    const auto read = Bitmap::read(out);
    ASSERT_TRUE(read);
    EXPECT_EQ(*read, written);
    for (int field = 1; field <= Bitmap::max_field; ++field) {
        EXPECT_EQ(read->test(field), written.test(field)) << "field " << field;
    }
}

TEST(Bitmap, ReadStopsAfterThePrimaryWhenBitOneIsClear) {
    const auto read = Bitmap::read(hex("7000000000000000"
                                       "FFFFFFFFFFFFFFFF"));
    ASSERT_TRUE(read);
    EXPECT_EQ(read->size_bytes(), 8U);
    EXPECT_FALSE(read->test(65));
}

TEST(Bitmap, ReadReportsATruncatedPrimary) {
    EXPECT_EQ(Bitmap::read(hex("40000000000000")).error(), (Error{Errc::truncated, 0, 0}));
}

TEST(Bitmap, ReadReportsATruncatedSecondary) {
    EXPECT_EQ(Bitmap::read(hex("8000000000000000"
                               "0000"))
                  .error(),
              (Error{Errc::truncated, 0, 8}));
}

} // namespace
} // namespace iso8583

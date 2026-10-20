#include <gtest/gtest.h>

#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>

#include "fixtures.hpp"
#include "iso8583/codec.hpp"
#include "iso8583/print.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

using test::hex;

std::string printed(const Message& message, const FieldTable& table = fields_1987) {
    std::ostringstream out;
    print(out, message, table);
    return out.str();
}

bool contains(const std::string& text, std::string_view part) {
    return text.find(part) != std::string::npos;
}

TEST(MaskPan, KeepsTheFirstSixAndLastFourDigits) {
    EXPECT_EQ(mask_pan("4111111111111111"), "411111******1111");
    EXPECT_EQ(mask_pan("6011000990139424123"), "601100*********4123");
    EXPECT_EQ(mask_pan("4222222222222"), "422222***2222");
    EXPECT_EQ(mask_pan("41111122222"), "411111*2222");
}

TEST(MaskPan, MasksAPanTooShortToHideAnythingCompletely) {
    EXPECT_EQ(mask_pan("4111111111"), "**********");
    EXPECT_EQ(mask_pan("4111"), "****");
    EXPECT_EQ(mask_pan(""), "");
}

TEST(Print, LaysOutHeaderBitmapAndOneLinePerField) {
    EXPECT_EQ(printed(test::financial_response()),
              "0210  financial request response (origin acquirer, ISO 8583:1987)\n"
              "bitmap 722000000E808000\n"
              "  002  n..19   Primary account number                  411111******1111\n"
              "  003  n6      Processing code                         000000\n"
              "  004  n12     Amount, transaction                     000000002500\n"
              "  007  n10     Transmission date and time              1020214501\n"
              "  011  n6      Systems trace audit number              000123\n"
              "  037  an12    Retrieval reference number              629400000123\n"
              "  038  an6     Authorization identification response   A1B2C3\n"
              "  039  an2     Response code                           00\n"
              "  041  ans8    Card acceptor terminal identification   TERM0001\n"
              "  049  n3      Currency code, transaction              036\n");
}

TEST(Print, NeverShowsTheClearPanOrThePinBlock) {
    const std::string text = printed(test::financial_request());
    EXPECT_TRUE(contains(text, "411111******1111"));
    EXPECT_FALSE(contains(text, "4111111111111111"));
    EXPECT_FALSE(contains(text, to_hex(test::pin_block())));
    EXPECT_TRUE(contains(text, "Personal identification number data     [redacted]"));
    EXPECT_TRUE(contains(text, to_hex(test::mac())));
    EXPECT_TRUE(contains(text, "bitmap F238000008C09000 0000000000000001\n"));
}

TEST(Print, RedactsFieldsTheTableDoesNotDefine) {
    Message message{Mti{"0200"}};
    message.set(70, "301");
    EXPECT_TRUE(contains(printed(message),
                         "  070  ?       (not in field table)                    [redacted]\n"));
}

TEST(Print, RedactsBytesStoredInThePanField) {
    Message message{Mti{"0200"}};
    message.set(2, hex("4111111111111111"));
    const std::string text = printed(message);
    EXPECT_TRUE(contains(text, "Primary account number                  [redacted]"));
    EXPECT_FALSE(contains(text, "4111111111111111"));
}

TEST(Print, UsesTheGivenTable) {
    Message message{Mti{"0200"}};
    message.set(48, "R01=Y");
    EXPECT_TRUE(contains(printed(message, test::extended_table), "Additional data, private"));
    EXPECT_TRUE(contains(printed(message), "(not in field table)"));
}

TEST(Print, LeavesTheCallersStreamFormattingAlone) {
    std::ostringstream out;
    out << std::hex << std::setfill('#');
    print(out, test::financial_response());
    EXPECT_TRUE(out.flags() & std::ios::hex);
    EXPECT_EQ(out.fill(), '#');
}

TEST(Print, StreamOperatorPrintsWithThe1987Table) {
    std::ostringstream out;
    out << test::financial_request();
    EXPECT_EQ(out.str(), printed(test::financial_request()));
}

} // namespace
} // namespace iso8583

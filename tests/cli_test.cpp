#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "cli.hpp"
#include "fixtures.hpp"
#include "iso8583/bytes.hpp"
#include "iso8583/codec.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

struct Outcome {
    int exit_code;
    std::string out;
    std::string err;
};

Outcome run_cli(const std::vector<std::string_view>& args, const std::string& input = {}) {
    std::istringstream in{input};
    std::ostringstream out;
    std::ostringstream err;
    const int exit_code = cli::run(args, in, out, err);
    return {exit_code, out.str(), err.str()};
}

TEST(Cli, SamplePrintsHexThatDecodes) {
    const Outcome outcome = run_cli({"sample"});
    ASSERT_EQ(outcome.exit_code, cli::exit_ok);

    const auto bytes = from_hex(outcome.out);
    ASSERT_TRUE(bytes) << outcome.out;
    const auto message = decode(*bytes);
    ASSERT_TRUE(message) << to_string(message.error());
    EXPECT_EQ(message->mti(), Mti{"0200"});
    EXPECT_TRUE(message->has(128));
}

TEST(Cli, DecodePrintsTheMessageWithThePanMasked) {
    const Outcome outcome = run_cli({"decode", to_hex(test::financial_request_wire())});
    EXPECT_EQ(outcome.exit_code, cli::exit_ok);
    EXPECT_NE(outcome.out.find("0200  financial request"), std::string::npos);
    EXPECT_NE(outcome.out.find("411111******1111"), std::string::npos);
    EXPECT_EQ(outcome.out.find("4111111111111111"), std::string::npos);
    EXPECT_TRUE(outcome.err.empty());
}

TEST(Cli, DecodeReadsHexFromStdinGivenADash) {
    const std::string spaced = "30 32 31 30\n" + to_hex(test::financial_response_wire()).substr(8);
    const Outcome outcome = run_cli({"decode", "-"}, spaced);
    EXPECT_EQ(outcome.exit_code, cli::exit_ok) << outcome.err;
    EXPECT_NE(outcome.out.find("Response code"), std::string::npos);
}

TEST(Cli, DecodeRejectsTextThatIsNotHex) {
    const Outcome outcome = run_cli({"decode", "30323030zz"});
    EXPECT_EQ(outcome.exit_code, cli::exit_bad_message);
    EXPECT_EQ(outcome.err, "error: input is not valid hex\n");
    EXPECT_TRUE(outcome.out.empty());
}

TEST(Cli, DecodeSaysWhereTheMessageIsBroken) {
    // "0200", a bitmap with field 2, then the length prefix "1X".
    const Outcome outcome = run_cli({"decode", "303230304000000000000000"
                                               "3158"});
    EXPECT_EQ(outcome.exit_code, cli::exit_bad_message);
    EXPECT_EQ(outcome.err, "error: field 2: length prefix is not numeric (offset 12)\n");
}

TEST(Cli, HelpGoesToStandardOutput) {
    for (const std::string_view flag : {"--help", "-h"}) {
        const Outcome outcome = run_cli({flag});
        EXPECT_EQ(outcome.exit_code, cli::exit_ok);
        EXPECT_EQ(outcome.out.rfind("usage: iso8583", 0), 0U);
        EXPECT_TRUE(outcome.err.empty());
    }
}

TEST(Cli, BadUsageGoesToStandardError) {
    const std::vector<std::vector<std::string_view>> misuses{
        {}, {"decode"}, {"sample", "extra"}, {"encode", "0200"}};
    for (const auto& args : misuses) {
        const Outcome outcome = run_cli(args);
        EXPECT_EQ(outcome.exit_code, cli::exit_usage);
        EXPECT_EQ(outcome.err.rfind("usage: iso8583", 0), 0U);
        EXPECT_TRUE(outcome.out.empty());
    }
}

} // namespace
} // namespace iso8583

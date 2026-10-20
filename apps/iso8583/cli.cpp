#include "cli.hpp"

#include <istream>
#include <iterator>
#include <ostream>
#include <string>

#include "iso8583/bytes.hpp"
#include "iso8583/codec.hpp"
#include "iso8583/message.hpp"
#include "iso8583/print.hpp"

namespace iso8583::cli {
namespace {

constexpr std::string_view usage =
    "usage: iso8583 decode <hex>   decode a message and print it; '-' reads the hex from stdin\n"
    "       iso8583 sample         print a sample 0200 financial request as hex\n";

// A purchase of AUD 25.00 with an online PIN, using the well-known Visa test
// PAN. Field 7 is GMT; fields 12 and 13 are the terminal's local time (AEDT).
// The MAC in field 128 forces a secondary bitmap.
Message sample_request() {
    Message request{Mti{"0200"}};
    request.set(2, "4111111111111111")
        .set(3, "000000")
        .set(4, "000000002500")
        .set(7, "1020214500")
        .set(11, "000123")
        .set(12, "084500")
        .set(13, "1021")
        .set(37, "629400000123")
        .set(41, "TERM0001")
        .set(42, "MERCHANT0000042")
        .set(49, "036")
        .set(52, Bytes{std::byte{0x9F}, std::byte{0x3A}, std::byte{0x51}, std::byte{0x0C},
                       std::byte{0xE2}, std::byte{0x77}, std::byte{0x84}, std::byte{0x1B}})
        .set(128, Bytes{std::byte{0x5B}, std::byte{0x8E}, std::byte{0x21}, std::byte{0xC4},
                        std::byte{0x07}, std::byte{0xF9}, std::byte{0x3D}, std::byte{0x62}});
    return request;
}

int decode_command(std::string_view argument, std::istream& in, std::ostream& out,
                   std::ostream& err) {
    std::string hex{argument};
    if (argument == "-") {
        hex.assign(std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{});
    }
    const auto bytes = from_hex(hex);
    if (!bytes) {
        err << "error: input is not valid hex\n";
        return exit_bad_message;
    }
    const auto message = decode(*bytes);
    if (!message) {
        err << "error: " << to_string(message.error()) << '\n';
        return exit_bad_message;
    }
    out << *message;
    return exit_ok;
}

} // namespace

int run(std::span<const std::string_view> args, std::istream& in, std::ostream& out,
        std::ostream& err) {
    if (args.size() == 2 && args[0] == "decode") {
        return decode_command(args[1], in, out, err);
    }
    if (args.size() == 1 && args[0] == "sample") {
        out << to_hex(encode(sample_request()).value()) << '\n';
        return exit_ok;
    }
    if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h")) {
        out << usage;
        return exit_ok;
    }
    err << usage;
    return exit_usage;
}

} // namespace iso8583::cli

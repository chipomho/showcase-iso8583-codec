# iso8583-codec

An encoder and decoder for ISO 8583:1987 card payment messages, in C++20 with
nothing but the standard library (GoogleTest for the tests).

It handles the message type indicator, the primary bitmap, the secondary
bitmap when any field from 65 to 128 is present, and a table of data elements
that covers a basic purchase and its response:

| Field | Name                                  | Format |
|-------|---------------------------------------|--------|
| 2     | Primary account number                | n..19  |
| 3     | Processing code                       | n6     |
| 4     | Amount, transaction                   | n12    |
| 7     | Transmission date and time            | n10    |
| 11    | Systems trace audit number            | n6     |
| 12/13 | Time / date, local transaction        | n6, n4 |
| 37    | Retrieval reference number            | an12   |
| 38    | Authorization identification response | an6    |
| 39    | Response code                         | an2    |
| 41    | Card acceptor terminal identification | ans8   |
| 42    | Card acceptor identification code     | ans15  |
| 49    | Currency code, transaction            | n3     |
| 52    | PIN data                              | b8     |
| 128   | Message authentication code           | b8     |

Numeric and text values and the LL/LLL length prefixes are ASCII; the bitmaps
and b fields are binary.

## Usage

```cpp
#include <iostream>

#include "iso8583/codec.hpp"
#include "iso8583/print.hpp"

int main() {
    iso8583::Message request{iso8583::Mti{"0200"}};
    request.set(2, "4111111111111111")
        .set(3, "000000")
        .set(4, "2500") // sent as 000000002500
        .set(11, "000123")
        .set(41, "TERM0001")
        .set(49, "036");

    const auto wire = iso8583::encode(request);
    if (!wire) {
        std::cerr << iso8583::to_string(wire.error()) << '\n';
        return 1;
    }

    const auto decoded = iso8583::decode(*wire);
    std::cout << decoded.value(); // the PAN prints as 411111******1111
}
```

`decode` takes a `std::span<const std::byte>` and returns either a `Message`
or an `Error` naming the problem, the field and the byte offset. It does not
throw on bad input.

## Command line

```
$ iso8583 sample | iso8583 decode -
0200  financial request (origin acquirer, ISO 8583:1987)
bitmap F238000008C09000 0000000000000001
  002  n..19   Primary account number                  411111******1111
  003  n6      Processing code                         000000
  004  n12     Amount, transaction                     000000002500
  007  n10     Transmission date and time              1020214500
  011  n6      Systems trace audit number              000123
  012  n6      Time, local transaction                 084500
  013  n4      Date, local transaction                 1021
  037  an12    Retrieval reference number              629400000123
  041  ans8    Card acceptor terminal identification   TERM0001
  042  ans15   Card acceptor identification code       MERCHANT0000042
  049  n3      Currency code, transaction              036
  052  b8      Personal identification number data     [redacted]
  128  b8      Message authentication code             5B8E21C407F93D62

$ iso8583 decode 30323030400000000000000031583431
error: field 2: length prefix is not numeric (offset 12)
```

`decode` takes the hex as an argument, or reads it from stdin given `-`.
Whitespace in the hex is ignored, so a dump pasted from a trace works. Exit
codes: 0 success, 1 the message could not be decoded, 2 usage error.

## Design notes

- **The field table is data.** Each field is a `FieldSpec` (format, length
  type, maximum length, how it may be displayed) in a `constexpr` array.
  `FieldTable`'s constructor is `consteval` and checks the table is sorted,
  within 2..128 and has lengths its prefixes can carry, so a bad table is a
  compile error. Another dialect is another table; the tests use one with
  LLLVAR text and EMV-style LLLVAR binary.
- **Errors are values.** `encode` and `decode` return a small `Result<T>`
  holding a value or an `Error`. `std::expected` would be the obvious choice
  but is C++23 and not available in GCC 11.
- **Bytes left after the last field are an error.** In practice they mean a
  framing fault, and that is better caught at the codec than three layers up.
- **Printing follows the field table.** The PAN is masked to the first six and
  last four digits, the PIN block is never shown, and fields the table does
  not define are redacted because nothing says they are safe. There is no
  printing path that shows the clear PAN.
- **Padding is applied on encode, not undone on decode.** Short fixed-length n
  values are zero-filled on the left and an/ans values space-filled on the
  right, as hosts expect. Decoding returns the wire value unchanged, so
  `decode(encode(m)) == m` holds whenever `m` already uses full-width values.
  Space is accepted in an fields for this reason.
- **MTI literals are checked at compile time.** `Mti{"02X0"}` does not build;
  `Mti::parse` is for text that arrives at run time. Two compile-fail tests
  make sure both of these checks keep failing for the right reason.

## Not covered

EBCDIC, BCD-packed numerics and binary length prefixes, all of which some host
dialects use; the tertiary bitmap (field 65 is rejected); the transport length
header, which belongs to the connection layer; MAC calculation and PIN block
translation (fields 52 and 128 are carried as opaque bytes); and semantic checks
such as whether field 7 is a real date.

## Building

Needs CMake 3.25 or later, Ninja and a C++20 compiler. It is built and tested
with MSVC 19.44 (Visual Studio 2022) and GCC 11.4. GoogleTest 1.17.0 is
downloaded at configure time and pinned by hash.

Windows, from an x64 Native Tools prompt (or after `vcvars64.bat`):

```
cmake --workflow --preset msvc-debug
cmake --workflow --preset msvc-release
```

Linux or WSL:

```
cmake --workflow --preset gcc-debug
cmake --workflow --preset gcc-release
cmake --workflow --preset gcc-asan       # AddressSanitizer + UndefinedBehaviorSanitizer
cmake --workflow --preset gcc-coverage   # needs gcovr; fails below 100% line coverage
```

Each workflow is configure, build and test; the steps can also be run
separately with `cmake --preset`, `cmake --build --preset` and
`ctest --preset`. Warnings are errors on both compilers.

Under WSL, build from a clone on the Linux filesystem. With the sources under
`/mnt/c` each test process is slow to start and the suite takes minutes
instead of seconds.

## Tests

- Known-answer vectors: wire bytes assembled by hand from the layout rules,
  not produced by the encoder, for a 0200 request (secondary bitmap) and its
  0210 response (primary only).
- Round trips in both directions, plus 1,300 random messages from a fixed
  seed.
- A table of malformed inputs, each checked for the exact error, field and
  offset; every truncation of a valid message; and bit flips across a valid
  message, which must either be rejected or decode to something that
  re-encodes.
- Character-set boundaries, PAN masking, the print layout and the CLI.

Line coverage is 100%. Branch coverage is about 73% as gcov counts it,
because GCC treats every call that might throw as a branch; with those edges
excluded it is 99%. The coverage target prints both figures. Its gcovr
options are explained in `cmake/CompilerOptions.cmake`.

## Layout

```
include/iso8583/   public headers
src/               implementation
apps/iso8583/      the command line tool
tests/             unit tests, compile-fail cases, CLI end-to-end check
cmake/             compiler options and coverage
```

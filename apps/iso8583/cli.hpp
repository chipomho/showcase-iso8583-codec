#pragma once

#include <iosfwd>
#include <span>
#include <string_view>

namespace iso8583::cli {

inline constexpr int exit_ok = 0;
inline constexpr int exit_bad_message = 1;
inline constexpr int exit_usage = 2;

// Runs the iso8583 command line tool. `args` excludes the program name.
// Returns the process exit code.
int run(std::span<const std::string_view> args, std::istream& in, std::ostream& out,
        std::ostream& err);

} // namespace iso8583::cli

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "cli.hpp"

int main(int argc, char* argv[]) {
    const std::span<char*> raw{argv, static_cast<std::size_t>(argc)};
    const std::vector<std::string_view> args(raw.begin() + std::min<std::ptrdiff_t>(1, argc),
                                             raw.end());
    return iso8583::cli::run(args, std::cin, std::cout, std::cerr);
}

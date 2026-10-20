// Must not compile: an MTI literal with a letter in it.
#include "iso8583/mti.hpp"

int main() {
    const iso8583::Mti mti{"02X0"};
    return mti.version();
}

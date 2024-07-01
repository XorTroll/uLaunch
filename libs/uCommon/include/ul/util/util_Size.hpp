
#pragma once
#include <switch.h>

namespace ul::util::size {

    inline constexpr size_t operator ""_KB(unsigned long long n) {
        return n * 0x400;
    }

    inline constexpr size_t operator ""_MB(unsigned long long n) {
        return operator ""_KB(n) * 0x400;
    }

    inline constexpr size_t operator ""_GB(unsigned long long n) {
        return operator ""_MB(n) * 0x400;
    }

}

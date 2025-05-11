
#pragma once
#include <switch.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace ul::util {

    template<typename S>
    inline const char *GetCString(const S &s) {
        return s;
    }
    
    template<>
    inline const char *GetCString<std::string>(const std::string &s) {
        return s.c_str();
    }

    inline bool StringEndsWith(const std::string &value, const std::string &ending) {
        if(ending.size() > value.size()) {
            return false;
        }
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    std::string FormatProgramId(const u64 program_id);

    inline u64 Get64FromString(const std::string &val) {
        return strtoull(val.c_str(), nullptr, 16);
    }

    template<size_t S>
    inline void CopyToStringBuffer(char (&dst)[S], const std::string &src) {
        const auto copy_size = std::min(S - 1, src.length());
        memcpy(dst, src.c_str(), copy_size);
        dst[copy_size] = '\0';
    }

    template<size_t S1, size_t S2>
    inline void CopyToStringBuffer(char (&dst)[S1], const char (&src)[S2]) {
        constexpr auto copy_size = std::min(S1 - 1, S2 - 1);
        memcpy(dst, src, copy_size);
        dst[copy_size] = '\0';
    }

    std::string FormatAccount(const AccountUid value);
    std::string FormatResultDisplay(const Result rc);

    std::string FormatSha256Hash(const u8 *hash, const bool only_half);

}


#pragma once
#include <switch.h>
#include <string>
#include <sstream>
#include <iomanip>

namespace ul::util {

    inline bool StringEndsWith(const std::string &value, const std::string &ending) {
        if(ending.size() > value.size()) {
            return false;
        }
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    inline std::string FormatProgramId(const u64 program_id) {
        std::stringstream strm;
        strm << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << program_id;
        return strm.str();
    }

    inline u64 Get64FromString(const std::string &val) {
        return strtoull(val.c_str(), nullptr, 16);
    }

    std::string FormatAccount(const AccountUid value);
    std::string FormatResultDisplay(const Result rc);
    std::string FormatResultHex(const Result rc);

}

#pragma once
#include <ul_Include.hpp>

namespace util {

    std::string Format128NintendoStyle(const AccountUid value);
    
    inline u64 Get64FromString(const std::string &val) {
        return strtoull(val.c_str(), nullptr, 16);
    }
    
    std::string FormatApplicationId(const u64 app_id);
    std::string FormatResultDisplay(const Result rc);
    std::string FormatResultHex(const Result rc);
    std::string FormatResult(const Result rc);

}
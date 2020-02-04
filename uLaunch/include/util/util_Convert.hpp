
#pragma once
#include <ul_Include.hpp>

namespace util
{
    std::string Format128NintendoStyle(AccountUid value);
    
    inline u64 Get64FromString(const std::string &val)
    {
        return strtoull(val.c_str(), nullptr, 16);
    }
    
    std::string FormatApplicationId(u64 app_id);
    std::string FormatResultDisplay(Result rc);
    std::string FormatResultHex(Result rc);
    std::string FormatResult(Result rc);
}
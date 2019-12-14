
#pragma once
#include <q_Include.hpp>

namespace util
{
    std::string Format128NintendoStyle(AccountUid value);
    u64 Get64FromString(std::string val);
    std::string FormatApplicationId(u64 app_id);
    std::string FormatResultDisplay(Result rc);
    std::string FormatResultHex(Result rc);
    std::string FormatResult(Result rc);
}
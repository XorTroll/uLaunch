
#pragma once
#include <q_Include.hpp>

namespace util
{
    std::string Format128NintendoStyle(u128 value);
    u64 Get64FromString(std::string val);
    std::string FormatApplicationId(u64 app_id);
}
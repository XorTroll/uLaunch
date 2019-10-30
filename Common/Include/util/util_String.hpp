
#pragma once
#include <q_Include.hpp>

namespace util
{
    inline bool StringEndsWith(std::string const &value, std::string const &ending)
    {
        if(ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }
}
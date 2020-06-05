
#pragma once
#include <ul_Include.hpp>

namespace util {

    inline bool StringEndsWith(const std::string &value, const std::string &ending) {
        if(ending.size() > value.size()) {
            return false;
        }
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

}
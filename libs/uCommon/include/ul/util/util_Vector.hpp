
#pragma once
#include <vector>

namespace ul::util {

    template<typename T>
    inline void VectorRemoveByValue(std::vector<T> &vec, const T &val) {
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end()); 
    }

}
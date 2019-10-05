#include <util/util_String.hpp>

namespace util
{
    bool StringStartsWith(const std::string &base, std::string str)
    {
        return (base.substr(0, str.length()) == str);
    }

    bool StringEndsWith(const std::string &base, std::string str)
    {
        return (base.substr(base.length() - str.length()) == str);
    }
}
#include <util/util_JSON.hpp>

namespace util
{
    ResultWith<JSON> LoadJSONFromFile(std::string path)
    {
        JSON ret = JSON::object();
        std::ifstream ifs(path);
        if(ifs.good())
        {
            ret = JSON::parse(ifs);
            return SuccessResultWith(ret);
        }
        return MakeResultWith(0xdead, ret);
    }
}
#include <util/util_Misc.hpp>
#include <fs/fs_Stdio.hpp>

namespace util {

    Result LoadJSONFromFile(JSON &out_json, const std::string &path) {
        if(fs::ExistsFile(path)) {
            try {
                std::ifstream ifs(path);
                out_json = JSON::parse(ifs);
                return ResultSuccess;
            }
            catch(...) {}
        }

        return misc::ResultInvalidJsonFile;
    }

}
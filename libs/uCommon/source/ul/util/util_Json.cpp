#include <ul/util/util_Json.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_Results.hpp>
#include <fstream>

namespace ul::util {

    Result LoadJSONFromFile(JSON &out_json, const std::string &path) {
        if(fs::ExistsFile(path)) {
            try {
                std::ifstream ifs(path);
                out_json = JSON::parse(ifs);
                return ResultSuccess;
            }
            catch(...) {}
        }

        return ResultInvalidJson;
    }

    void SaveJSON(const std::string &path, const JSON &json) {
        // FIX: avoid using streams?
        std::ofstream ofs(path);
        ofs << std::setw(4) << json;
        ofs.close();
    }

}
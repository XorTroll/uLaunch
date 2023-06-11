#include <ul/util/util_Json.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_Results.hpp>

namespace ul::util {

    Result LoadJSONFromFile(JSON &out_json, const std::string &path) {
        if(fs::ExistsFile(path)) {
            std::string json_str;
            if(fs::ReadFileString(path, json_str)) {
                try {
                    out_json = JSON::parse(json_str);
                }
                catch(std::exception&) {}
                return ResultSuccess;
            }
        }

        return ResultInvalidJson;
    }

    bool SaveJSON(const std::string &path, const JSON &json) {
        const auto json_str = json.dump(4);
        return fs::WriteFileString(path, json_str, true);
    }

}
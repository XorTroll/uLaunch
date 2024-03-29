
#pragma once
#include <extras/json.hpp>

namespace ul::util {

    using JSON = nlohmann::json;

    Result LoadJSONFromFile(JSON &out_json, const std::string &path);
    bool SaveJSON(const std::string &path, const JSON &json);

}
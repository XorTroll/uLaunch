
#pragma once
#include <extras/json.hpp>
#include <switch.h>

namespace ul::util {

    using JSON = nlohmann::json;

    Result LoadJSONFromFile(JSON &out_json, const std::string &path);
    void SaveJSON(const std::string &path, const JSON &json);

}
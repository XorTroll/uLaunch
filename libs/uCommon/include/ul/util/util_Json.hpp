
#pragma once
#include <ul/ul_Result.hpp>
#define JSON_THROW_USER(exception) ::ul::OnAssertionFailed(::rc::ulaunch::ResultAssertionFailed, "JSON libraries threw " #exception "...\n")
#define JSON_TRY_USER if(true)
#define JSON_CATCH_USER(exception) if(false)
#define JSON_INTERNAL_CATCH_USER(exception) if(false)
#include <json.hpp>

namespace ul::util {

    using JSON = nlohmann::json;

    Result LoadJSONFromFile(JSON &out_json, const std::string &path);
    bool SaveJSON(const std::string &path, const JSON &json);

}


#pragma once
#include <switch.h>
#include <string>
#include <cerrno>
#include <tuple>
#include <cstdlib>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <sstream>
#include <cinttypes>
#include <iomanip>
#include <map>
#include <fstream>
#include <functional>
#include <thread>

// JSON utils

#include <json.hpp>
using JSON = nlohmann::json;

// Library defines

#define UL_BASE_DIR "ulaunch"
#define UL_BASE_SD_DIR "sdmc:/" UL_BASE_DIR
#define UL_DB_MOUNT_NAME "ul_save"
#define UL_DB_MOUNT_PATH UL_DB_MOUNT_NAME ":/"
#define UL_BASE_DB_DIR UL_DB_MOUNT_PATH UL_BASE_DIR
#define UL_ENTRIES_PATH UL_BASE_SD_DIR "/entries"
#define UL_THEMES_PATH UL_BASE_SD_DIR "/themes"
#define UL_NRO_CACHE_PATH UL_BASE_SD_DIR "/nro"
#define UL_TITLE_CACHE_PATH UL_BASE_SD_DIR "/titles"
#define UL_ASSERTION_LOG_FILE UL_BASE_SD_DIR "/err.log"

#ifndef UL_VERSION
#error uLaunch's build version isn't defined
#endif

// Scope utils

#include <ul_Scope.hpp>

// Size utils

static constexpr size_t PlainRgbaScreenBufferSize = 1280 * 720 * 4;

inline constexpr size_t operator ""_KB(unsigned long long n) {
    return n * 0x400;
}

inline constexpr size_t operator ""_MB(unsigned long long n) {
    return operator ""_KB(n) * 0x400;
}

inline constexpr size_t operator ""_GB(unsigned long long n) {
    return operator ""_MB(n) * 0x400;
}

// STL utils

#define STL_FIND_IF(stl_item, var_name, cond) std::find_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); })
#define STL_FOUND(stl_item, find_ret) (find_ret != stl_item.end())
#define STL_UNWRAP(find_ret) (*find_ret)
#define STL_REMOVE_IF(stl_item, var_name, cond) stl_item.erase(std::remove_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); }), stl_item.end())

// Result and assertion utils

#include <ul_Result.hpp>

template<typename ...Args>
inline void NX_NORETURN OnAssertionFailed(const char *log_fmt, Args &&...args) {
    // TODO: unique log file for each assertion faial (a la crash report?)
    auto log_f = fopen(UL_ASSERTION_LOG_FILE, "wb"); \
    if(log_f) {
        fprintf(log_f, log_fmt, args...);
        fprintf(log_f, "\n");
        fclose(log_f);
    }

    fatalThrow(misc::ResultAssertionFailed);
}

#define UL_RC_ASSERT(expr) ({ \
    const auto _tmp_rc = res::TransformIntoResult(expr); \
    if(R_FAILED(_tmp_rc)) { \
        OnAssertionFailed("%s asserted 0x%X...", #expr, _tmp_rc); \
    } \
})

#define UL_ASSERT_TRUE(expr) ({ \
    const auto _tmp_expr = (expr); \
    if(!_tmp_expr) { \
        OnAssertionFailed("%s asserted to be false...", #expr); \
    } \
})

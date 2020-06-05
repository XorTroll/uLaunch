
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

#include <json.hpp>
using JSON = nlohmann::json;

#define UL_BASE_DIR "ulaunch"
#define UL_BASE_SD_DIR "sdmc:/" UL_BASE_DIR
#define UL_DB_MOUNT_NAME "ul_save"
#define UL_DB_MOUNT_PATH UL_DB_MOUNT_NAME ":/"
#define UL_BASE_DB_DIR UL_DB_MOUNT_PATH UL_BASE_DIR
#define UL_ENTRIES_PATH UL_BASE_SD_DIR "/entries"
#define UL_THEMES_PATH UL_BASE_SD_DIR "/themes"

#ifndef UL_VERSION
#error uLaunch's release version isn't defined.
#endif

#include <ul_Scope.hpp>

static constexpr size_t RawRGBAScreenBufferSize = 1280 * 720 * 4;

#ifndef R_TRY

// Thanks SciresM
#define R_TRY(res_expr) \
({ \
    const Result _tmp_r_try_rc = res_expr; \
    if (R_FAILED(_tmp_r_try_rc)) { \
        return _tmp_r_try_rc; \
    } \
})

#endif

#define R_TRY_WITH(res_expr, ...) \
({ \
    const Result _tmp_r_try_rc = res_expr; \
    if (R_FAILED(_tmp_r_try_rc)) { \
        return MakeResultWith(_tmp_r_try_rc, __VA_ARGS__); \
    } \
})

#define STL_FIND_IF(stl_item, var_name, cond) std::find_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); });
#define STL_FOUND(stl_item, find_ret) (find_ret != stl_item.end())
#define STL_UNWRAP(find_ret) (*find_ret)
#define STL_REMOVE_IF(stl_item, var_name, cond) stl_item.erase(std::remove_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); }), stl_item.end());

template<typename ...Args>
using ResultWith = std::tuple<Result, Args...>;

template<typename ...Args>
inline constexpr ResultWith<Args...> MakeResultWith(Result rc, Args &&...args)
{
    return std::forward_as_tuple(rc, args...);
}

template<typename ...Args>
inline constexpr ResultWith<Args...> SuccessResultWith(Args &&...args)
{
    return MakeResultWith(0, args...);
}

static constexpr Result ResultSuccess = 0;

static constexpr Mutex EmptyMutex = (Mutex)0;

#include <ul_Results.hpp>

#define UL_ASSERT(expr) ({ \
    const auto _tmp_rc = (expr); \
    if(R_FAILED(_tmp_rc)) { \
        fatalThrow(_tmp_rc); \
    } \
})

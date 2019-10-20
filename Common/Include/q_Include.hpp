
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

#include <json.hpp>
using JSON = nlohmann::json;

#define Q_BASE_DIR "reqwrite"
#define Q_BASE_SD_DIR "sdmc:/" Q_BASE_DIR
#define Q_DB_MOUNT_NAME "qsave"
#define Q_DB_MOUNT_PATH Q_DB_MOUNT_NAME ":/"
#define Q_BASE_DB_DIR Q_DB_MOUNT_PATH Q_BASE_DIR
#define Q_ENTRIES_PATH Q_BASE_SD_DIR "/entries"
#define Q_THEMES_PATH Q_BASE_SD_DIR "/themes"

#ifndef Q_VERSION
#error Project's version isn't defined.
#endif

// Thanks SciresM
#define R_TRY(res_expr) \
({ \
    const Result _tmp_r_try_rc = res_expr; \
    if (R_FAILED(_tmp_r_try_rc)) { \
        return _tmp_r_try_rc; \
    } \
})

#define R_TRY_WITH(res_expr, ...) \
({ \
    const Result _tmp_r_try_rc = res_expr; \
    if (R_FAILED(_tmp_r_try_rc)) { \
        return MakeResultWith(_tmp_r_try_rc, __VA_ARGS__); \
    } \
})

#define STLITER_FINDWITHCONDITION(stl_item, var_name, cond) std::find_if(stl_item.begin(), stl_item.end(), [&](auto &var_name){ return (cond); });
#define STLITER_ISFOUND(stl_item, find_ret) (find_ret != stl_item.end())
#define STLITER_UNWRAP(find_ret) (*find_ret)

template<typename ...Args>
using ResultWith = std::tuple<Result, Args...>;

template<typename ...Args>
constexpr ResultWith<Args...> MakeResultWith(Result rc, Args &&...args)
{
    return std::forward_as_tuple(rc, args...);
}

template<typename ...Args>
constexpr ResultWith<Args...> SuccessResultWith(Args &&...args)
{
    return MakeResultWith(0, args...);
}

// Console

#include <iostream>

#define CONSOLE_OUT(...) { std::cout << __VA_ARGS__ << std::endl; consoleUpdate(NULL); }
#define CONSOLE_FMT(fmt, ...) { printf(fmt "\n", ##__VA_ARGS__); consoleUpdate(NULL); }
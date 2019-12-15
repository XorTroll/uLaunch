
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

#define Q_BASE_DIR "ulaunch"
#define Q_BASE_SD_DIR "sdmc:/" Q_BASE_DIR
#define Q_MENU_ROMFS Q_BASE_SD_DIR "/bin/QMenu/romfs.bin"
#define Q_DB_MOUNT_NAME "qsave"
#define Q_DB_MOUNT_PATH Q_DB_MOUNT_NAME ":/"
#define Q_BASE_DB_DIR Q_DB_MOUNT_PATH Q_BASE_DIR
#define Q_ENTRIES_PATH Q_BASE_SD_DIR "/entries"
#define Q_THEMES_PATH Q_BASE_SD_DIR "/themes"

#ifndef Q_VERSION
#error uLaunch's release version isn't defined.
#endif

static constexpr size_t RawRGBAScreenBufferSize = 1280 * 720 * 4;

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

#include <q_Results.hpp>

// Console (debug)

#include <iostream>

#define CONSOLE_OUT(...) { std::cout << __VA_ARGS__ << std::endl; consoleUpdate(NULL); }
#define CONSOLE_FMT(fmt, ...) { printf(fmt "\n", ##__VA_ARGS__); consoleUpdate(NULL); }

inline void Panic(std::string msg)
{
    /*
    consoleInit(NULL);
    CONSOLE_OUT("")
    CONSOLE_OUT("uLaunch - main menu panic")
    CONSOLE_OUT("Panic error: " << msg)
    CONSOLE_OUT("")
    CONSOLE_OUT("Press any key to shutdown the console...")

    while(true)
    {
        hidScanInput();
        if(hidKeysDown(CONTROLLER_P1_AUTO))
        {
            consoleExit(NULL);
            bpcInitialize();
            bpcShutdownSystem();
            bpcExit();
            while(true);

            break;
        }
    }

    consoleExit(NULL);
    */

    // TODO: non-console panic...?
}

#define Q_R_TRY(expr) { auto _tmp_rc = (expr); if(R_FAILED(_tmp_rc)) { Panic("'" #expr "' failed: " + util::FormatResultDisplay(_tmp_rc)); } }
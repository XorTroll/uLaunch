
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
#error uLaunch's build version isn't defined
#endif

#include <ul_Scope.hpp>

static constexpr size_t RawRGBAScreenBufferSize = 1280 * 720 * 4;

#define STL_FIND_IF(stl_item, var_name, cond) std::find_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); })
#define STL_FOUND(stl_item, find_ret) (find_ret != stl_item.end())
#define STL_UNWRAP(find_ret) (*find_ret)
#define STL_REMOVE_IF(stl_item, var_name, cond) stl_item.erase(std::remove_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); }), stl_item.end())

#include <ul_Result.hpp>

inline constexpr size_t operator ""_KB(unsigned long long n) {
    return n * 0x400;
}

inline constexpr size_t operator ""_MB(unsigned long long n) {
    return operator ""_KB(n) * 0x400;
}

inline constexpr size_t operator ""_GB(unsigned long long n) {
    return operator ""_MB(n) * 0x400;
}

#define UL_ASSERTION_LOG_FILE UL_BASE_SD_DIR "/err.log"

inline void NORETURN OnAssertionFailed(const char *log_buf, size_t log_buf_len, const Result rc) {
    auto log_f = fopen(UL_ASSERTION_LOG_FILE, "wb"); \
    if(log_f) {
        fwrite(log_buf, 1, log_buf_len, log_f);
        fclose(log_f);
    }

    fatalThrow(rc);
}

#define UL_ASSERT_LOG_LEN 0x400

#define UL_ASSERT(expr) ({ \
    const auto _tmp_rc = (expr); \
    if(R_FAILED(_tmp_rc)) { \
        char log_buf[UL_ASSERT_LOG_LEN] = {}; \
        const auto log_buf_len = std::sprintf(log_buf, "%s asserted 0x%X...", #expr, _tmp_rc); \
        OnAssertionFailed(log_buf, log_buf_len, _tmp_rc); \
    } \
})

#define UL_ASSERT_TRUE(expr) ({ \
    const auto _tmp_expr = (expr); \
    if(!_tmp_expr) { \
        char logbuf[UL_ASSERT_LOG_LEN] = {}; \
        const auto log_buf_len = sprintf(logbuf, "%s failed...", #expr); \
        OnAssertionFailed(logbuf, log_buf_len, 0xDEAD); \
    } \
})

#define UL_AMS_ASSERT(expr) UL_ASSERT(ams::Result(expr).GetValue())

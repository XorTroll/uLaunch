#include <ul/ul_Result.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <cstdarg>

namespace ul {

    namespace {

        char g_LogPath[FS_MAX_PATH] = {};
        RecursiveMutex g_LogLock;

        inline const char *FormatLogKind(const LogKind kind) {
            switch(kind) {
                case LogKind::Information: {
                    return "INFO";
                }
                case LogKind::Warning: {
                    return "WARN";
                }
                case LogKind::Critical: {
                    return "ERROR";
                }
                default: {
                    return "UNK";
                }
            }
        }

    }

    // Doing this since the process may not even have these initialized (looking at you, uLoader...)
    #define _UL_DO_WITH_FSDEV(...) ({ \
        const auto needs_sm = !serviceIsActive(smGetServiceSession()); \
        const auto needs_fs = !serviceIsActive(fsGetServiceSession()); \
        const auto needs_fsdev = fsdevGetDeviceFileSystem("sdmc") == nullptr; \
        if(!needs_sm || R_SUCCEEDED(smInitialize())) { \
            if(!needs_fs || R_SUCCEEDED(fsInitialize())) { \
                if(!needs_fsdev || R_SUCCEEDED(fsdevMountSdmc())) { \
                    { __VA_ARGS__ } \
                    if(needs_fsdev) { \
                        fsdevUnmountAll(); \
                    } \
                } \
                if(needs_fs) { \
                    fsExit(); \
                } \
            } \
            if(needs_sm) { \
                smExit(); \
            } \
        } \
    })

    void InitializeLogging(const char *proc_name) {
        snprintf(g_LogPath, sizeof(g_LogPath), "%s/log_%s.log", RootPath, proc_name);

        _UL_DO_WITH_FSDEV({
            remove(g_LogPath);
        });
    }

    void LogImpl(const LogKind kind, const char *log_fmt, ...) {
        ScopedLock lk(g_LogLock);

        if(g_LogPath[0] == '\0') {
            return;
        }

        va_list args;
        va_start(args, log_fmt);
        _UL_DO_WITH_FSDEV({
            auto file = fopen(g_LogPath, "ab+");
            if(file) {
                const auto kind_str = FormatLogKind(kind);
                fprintf(file, "[%s] ", kind_str);
                vfprintf(file, log_fmt, args);
                fprintf(file, "\n");
                fflush(file);
                fclose(file);
            }
        });
        va_end(args);
    }

}

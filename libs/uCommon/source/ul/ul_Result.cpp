#include <ul/ul_Result.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <cstdarg>

extern "C" {

    void diagAbortWithResult(Result rc) {
        UL_RC_LOG_ASSERT("diagAbortWithResult", rc);
        __builtin_unreachable();
    }

}

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

        constexpr auto MaxLogFileCount = 10;

        inline void FormatLogPath(char *out_path, const char *proc_name, const u32 log_idx) {
            if(log_idx == 0) {
                snprintf(out_path, FS_MAX_PATH, "%s/log_%s.log", RootPath, proc_name);
            }
            else {
                snprintf(out_path, FS_MAX_PATH, "%s/log_%s_%d.log", RootPath, proc_name, log_idx);
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
        char tmp_log_path[FS_MAX_PATH] = {};

        _UL_DO_WITH_FSDEV({
            u32 i = MaxLogFileCount - 1;
            while(true) {
                FormatLogPath(g_LogPath, proc_name, i);
                if(fs::ExistsFile(g_LogPath)) {
                    if(i == MaxLogFileCount - 1) {
                        fs::DeleteFile(g_LogPath);
                    }
                    else {
                        FormatLogPath(tmp_log_path, proc_name, i + 1);
                        fs::RenameFile(g_LogPath, tmp_log_path);
                    }
                }

                if(i == 0) {
                    break;
                }
                i--;
            }

            // Loop ends with idx 0 formatted, which is what we want
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

    void AbortImpl(const Result rc) {
        svcBreak(BreakReason_Panic, reinterpret_cast<uintptr_t>(&rc), sizeof(rc));
        __builtin_unreachable();
    }

}

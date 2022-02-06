
#pragma once
#include <ul_Include.hpp>

#define UL_HB_HBTARGET_MAGIC "UHBT"
#define UL_HB_HBTARGET_MAGIC_U32 0x54424855

namespace hb {

    struct HbTargetParams {
        u32 magic;
        char nro_path[FS_MAX_PATH];
        char nro_argv[FS_MAX_PATH];
        bool target_once;

        inline std::string GetNROPath() {
            return this->nro_path;
        }

        inline std::string GetNROArgv() {
            return this->nro_argv;
        }

        inline std::string FormatToArgvString() {
            std::string argv = UL_HB_HBTARGET_MAGIC;
            std::string nro_path_copy = this->nro_path;
            std::replace(nro_path_copy.begin(), nro_path_copy.end(), ' ', (char)0xFF);
            argv += " " + nro_path_copy;
            std::string nro_argv_copy = this->nro_argv;
            std::replace(nro_argv_copy.begin(), nro_argv_copy.end(), ' ', (char)0xFF);
            argv += " " + nro_argv_copy;
            const auto target_once_num = this->target_once ? 1 : 0;
            argv += " " + std::to_string(target_once_num);
            return argv;
        }

        static inline HbTargetParams Create(const std::string &nro_path, const std::string &nro_argv, const bool target_once) {
            HbTargetParams params = {
                .magic = UL_HB_HBTARGET_MAGIC_U32,
                .target_once = target_once
            };

            strcpy(params.nro_path, nro_path.c_str());
            strcpy(params.nro_argv, nro_argv.c_str());
            return params;
        }
    };

}
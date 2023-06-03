
#pragma once
#include <switch.h>
#include <string>
#include <cstring>

namespace ul::loader {

    struct TargetInput {
        static constexpr u32 Magic = 0x52444C55; // "ULDR"

        u32 magic;
        char nro_path[FS_MAX_PATH];
        char nro_argv[FS_MAX_PATH];
        bool target_once;

        static inline TargetInput Create(const std::string &nro_path, const std::string &nro_argv, const bool target_once) {
            TargetInput target_ipt = {
                .magic = Magic,
                .target_once = target_once
            };

            strcpy(target_ipt.nro_path, nro_path.c_str());
            strcpy(target_ipt.nro_argv, nro_argv.c_str());
            return target_ipt;
        }

        inline bool IsValid() {
            return this->magic == Magic;
        }
    };

}
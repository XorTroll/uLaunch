
#pragma once
#include <ul/util/util_String.hpp>

namespace ul::loader {

    constexpr size_t NroPathSize = 512;
    constexpr size_t NroArgvSize = 2048;

    // We take advantage of this "text" support hbloader/hbmenu have, uMenu can show custom captions

    constexpr size_t MenuCaptionSize = 1024;
    constexpr char MenuCaptionHeader[] = "Loaded by uLoader v" UL_VERSION " - uLaunch's custom hbloader replacement ;)";

    struct TargetInput {
        static constexpr u32 Magic = 0x49444C55; // "ULDI"

        u32 magic;
        char nro_path[NroPathSize];
        char nro_argv[NroArgvSize];
        char menu_caption[MenuCaptionSize];
        bool target_once;

        template<typename S1, typename S2, typename S3>
        static inline TargetInput Create(const S1 &nro_path, const S2 &nro_argv, const bool target_once, const S3 &menu_caption) {
            TargetInput target_ipt = {
                .magic = Magic,
                .target_once = target_once
            };


            util::CopyToStringBuffer(target_ipt.nro_path, nro_path);
            util::CopyToStringBuffer(target_ipt.nro_argv, nro_argv);

            std::string menu_full_caption = menu_caption;
            if(!menu_full_caption.empty()) {
                menu_full_caption = "\n" + menu_full_caption;
            }
            menu_full_caption = MenuCaptionHeader + menu_full_caption;
            util::CopyToStringBuffer(target_ipt.menu_caption, menu_full_caption);
            return target_ipt;
        }

        inline bool IsValid() {
            return this->magic == Magic;
        }
    };

    struct TargetOutput {
        static constexpr u32 Magic = 0x4F444C55; // "ULDO"

        u32 magic;
        char nro_path[NroPathSize];
        char nro_argv[NroArgvSize];

        template<typename S1, typename S2>
        static inline TargetOutput Create(const S1 &nro_path, const S2 &nro_argv) {
            TargetOutput target_opt = {
                .magic = Magic
            };

            util::CopyToStringBuffer(target_opt.nro_path, nro_path);
            util::CopyToStringBuffer(target_opt.nro_argv, nro_argv);
            return target_opt;
        }

        inline bool IsValid() {
            return this->magic == Magic;
        }
    };

}

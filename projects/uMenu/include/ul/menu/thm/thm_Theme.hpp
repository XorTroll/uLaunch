
#pragma once
#include <switch.h>
#include <string>
#include <vector>

namespace ul::menu::thm {

    enum class MenuId {
        MainMenu
    };

    inline bool ConvertMenuIdToString(const MenuId id, std::string &out_str) {
        #define _UL_THEME_FOREACH_MENU(menu) \
        case MenuId::menu: { \
            out_str = #menu; \
            return true; \
        }

        switch(id) {
            _UL_THEME_FOREACH_MENU(MainMenu)

            default: {
                break;
            }
        }

        #undef _UL_THEME_FOREACH_MENU

        return false;
    }

    inline bool ConvertStringToMenuId(const std::string &str, MenuId &out_id) {
        #define _UL_THEME_FOREACH_MENU(menu) \
        if(str == #menu) { \
            out_id = MenuId::menu; \
            return true; \
        }

        _UL_THEME_FOREACH_MENU(MainMenu)

        #undef _UL_THEME_FOREACH_MENU

        return false;
    }

    struct ThemeManifest {
        std::string name;
        u32 format_version;
        std::string release;
        std::string description;
        std::string author;
    };

    struct Theme {
        ThemeManifest manifest;
        std::string path;

        inline bool IsValid() {
            return !this->path.empty();
        }

        std::string LoadMenuSource(const MenuId id);
    };

    constexpr const char ThemeDirectory[] = "sdmc:/umad/themes";

    std::vector<Theme> LoadThemes();

}
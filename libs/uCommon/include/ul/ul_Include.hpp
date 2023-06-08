
#pragma once
#include <switch.h>
#include <vector>
#include <string>
#include <unistd.h>

namespace ul {

    constexpr const char DefaultThemePath[] = "romfs:/default";
    constexpr const char DefaultLanguagePath[] = "romfs:/en.json";

    constexpr const char ConfigPath[] = "sdmc:/ulaunch/config.cfg";
    constexpr const char ThemesPath[] = "sdmc:/ulaunch/themes";
    constexpr const char EntriesPath[] = "sdmc:/ulaunch/themes";
    constexpr const char TitleCachePath[] = "sdmc:/ulaunch/titles";
    constexpr const char AccountCachePath[] = "sdmc:/ulaunch/user";
    constexpr const char EntryCachePath[] = "sdmc:/ulaunch/nro";

    constexpr const char RootHomebrewPath[] = "sdmc:/switch";

    inline std::string JoinPath(const std::string &a, const std::string &b) {
        return a + "/" + b;
    }

}

#pragma once
#include <ul/ul_Include.hpp>

namespace ul::os {

    constexpr const char *LanguageNameList[] = {
        "Japanese",
        "American English",
        "Français",
        "Deutsch",
        "Italiano",
        "Español",
        "Chinese",
        "Korean",
        "Nederlands",
        "Português",
        "Русский",
        "Taiwanese",
        "British English",
        "Français canadien",
        "Español latino",
        "Chinese (simplified)",
        "Chinese (traditional)",
        "Português brasileiro"
    };
    constexpr size_t LanguageNameCount = std::size(LanguageNameList);

    inline SetLanguage GetSystemLanguage() {
        u64 lang_code = 0;
        auto lang = SetLanguage_ENUS;
        setGetSystemLanguage(&lang_code);
        setMakeLanguage(lang_code, &lang);
        return lang;
    }

    u32 GetBatteryLevel();
    bool IsConsoleCharging();

    void GetCurrentTime(u32 &out_h, u32 &out_min, u32 &out_sec);
    std::string GetCurrentDate(const std::vector<std::string> &weekday_list);

}

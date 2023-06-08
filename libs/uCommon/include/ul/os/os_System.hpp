
#pragma once
#include <ul/ul_Include.hpp>

namespace ul::os {

    enum class Language : u32 {
        Japanese,
        AmericanEnglish,
        French,
        German,
        Italian,
        Spanish,
        Chinese,
        Korean,
        Dutch,
        Portuguese,
        Russian,
        Taiwanese,
        BritishEnglish,
        CanadianFrench,
        LatinAmericanSpanish,
        SimplifiedChinese,
        TraditionalChinese
    };

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
        setGetLanguageCode(&lang_code);
        setMakeLanguage(lang_code, &lang);
        return lang;
    }

    u32 GetBatteryLevel();
    bool IsConsoleCharging();
    std::string GetCurrentTime();

}
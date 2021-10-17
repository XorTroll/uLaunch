
#pragma once
#include <ul_Include.hpp>

namespace os {

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
    constexpr size_t LanguageNameCount = sizeof(LanguageNameList) / sizeof(const char*);

    u32 GetBatteryLevel();
    bool IsConsoleCharging();
    std::string GetCurrentTime();

}
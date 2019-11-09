
#pragma once
#include <q_Include.hpp>

namespace os
{
    enum class Language : u32
    {
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

    static std::vector<std::string> LanguageCodes =
    {
        "ja",
        "en-US",
        "fr",
        "de",
        "it",
        "es",
        "zh-CN",
        "ko",
        "nl",
        "pt",
        "ru",
        "zh-TW",
        "en-GB",
        "fr-CA",
        "es-419",
        "zn-Hans",
        "zn-Hant"
    };

    static std::vector<std::string> LanguageNames =
    {
        "日本語",
        "American English",
        "Français",
        "Deutsch",
        "Italiano",
        "Español",
        "中文",
        "한국어",
        "Nederlands",
        "Português",
        "Русский",
        "中文",
        "British English",
        "Français canadien",
        "Español latino",
        "中文",
        "中文"
    };

    u32 GetBatteryLevel();
    bool IsConsoleCharging();
    std::string GetFirmwareVersion();
}
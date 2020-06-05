
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

    std::string GetLanguageName(u32 idx);
    std::vector<std::string> &GetLanguageNameList();

    u32 GetBatteryLevel();
    bool IsConsoleCharging();
    std::string GetFirmwareVersion();
    std::string GetCurrentTime();

}
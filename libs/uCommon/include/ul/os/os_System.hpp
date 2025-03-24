
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

    struct Time {
        u32 h;
        u32 min;

        constexpr Time() : h(UINT32_MAX), min(UINT32_MAX) {}
        constexpr Time(const u32 h, const u32 min) : h(h), min(min) {}

        inline bool operator==(const Time &other) const {
            return (this->h == other.h) && (this->min == other.min);
        }

        inline bool operator!=(const Time &other) const {
            return !(*this == other);
        }
    };

    Time GetCurrentTime();

    struct Date {
        u32 day;
        u32 weekday_idx;
        u32 month;

        constexpr Date() : day(UINT32_MAX), weekday_idx(UINT32_MAX), month(UINT32_MAX) {}
        constexpr Date(const u32 day, const u32 weekday_idx, const u32 month) : day(day), weekday_idx(weekday_idx), month(month) {}

        inline bool operator==(const Date &other) const {
            return (this->day == other.day) && (this->weekday_idx == other.weekday_idx) && (this->month == other.month);
        }

        inline bool operator!=(const Date &other) const {
            return !(*this == other);
        }
    };

    Date GetCurrentDate();

}

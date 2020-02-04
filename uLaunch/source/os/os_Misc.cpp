#include <os/os_Misc.hpp>

namespace os
{
    static std::vector<std::string> g_lang_names =
    {
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
        "Chinese (traditional)"
    };

    std::string GetLanguageName(u32 idx)
    {
        if(idx >= g_lang_names.size()) return "";
        return g_lang_names[idx];
    }

    std::vector<std::string> &GetLanguageNameList()
    {
        return g_lang_names;
    }

    u32 GetBatteryLevel()
    {
        u32 lvl = 0;
        psmGetBatteryChargePercentage(&lvl);
        return lvl;
    }

    bool IsConsoleCharging()
    {
        ChargerType cht = ChargerType_None;
        psmGetChargerType(&cht);
        return (cht > ChargerType_None);
    }

    std::string GetFirmwareVersion()
    {
        SetSysFirmwareVersion fwver;
        setsysGetFirmwareVersion(&fwver);
        return fwver.display_version;
    }
}
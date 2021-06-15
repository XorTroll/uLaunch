#include <os/os_Misc.hpp>
#include <ctime>

namespace os {

    namespace {

        const std::vector<std::string> g_LanguageNameList = {
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

    }

    std::string GetLanguageName(u32 idx) {
        if(idx >= g_LanguageNameList.size()) {
            return "";
        }
        return g_LanguageNameList[idx];
    }

    const std::vector<std::string> &GetLanguageNameList() {
        return g_LanguageNameList;
    }

    u32 GetBatteryLevel() {
        u32 lvl = 0;
        psmGetBatteryChargePercentage(&lvl);
        return lvl;
    }

    bool IsConsoleCharging() {
        auto charger_type = PsmChargerType_Unconnected;
        psmGetChargerType(&charger_type);
        return charger_type > PsmChargerType_Unconnected;
    }

    std::string GetFirmwareVersion() {
        SetSysFirmwareVersion fwver;
        setsysGetFirmwareVersion(&fwver);
        return fwver.display_version;
    }

    // Thanks Goldleaf
    
    std::string GetCurrentTime() {
        auto time_val = time(nullptr);
        auto local_time = localtime(&time_val);
        auto h = local_time->tm_hour;
        auto min = local_time->tm_min;
        char str[0x10] = {};
        sprintf(str, "%02d:%02d", h, min);
        return str;
    }

}
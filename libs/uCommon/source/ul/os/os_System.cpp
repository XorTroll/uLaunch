#include <ul/os/os_System.hpp>
#include <ul/ul_Result.hpp>
#include <ctime>

namespace ul::os {

    namespace {

        constexpr u32 ExosphereApiVersionConfigItem = 65000;
        constexpr u32 ExosphereEmuMMCType = 65007;

        bool g_GotIsEmuMMC = false;
        bool g_IsEmuMMC = false;
    
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
    
    Time GetCurrentTime() {
        const auto time_val = time(nullptr);
        const auto local_time = localtime(&time_val);

        return Time(static_cast<u32>(local_time->tm_hour), static_cast<u32>(local_time->tm_min), static_cast<u32>(local_time->tm_sec));
    }

    Date GetCurrentDate() {
        const auto time_val = time(nullptr);
        const auto local_time = localtime(&time_val);

        return Date(static_cast<u32>(local_time->tm_mday), static_cast<u32>(local_time->tm_wday), static_cast<u32>(local_time->tm_mon + 1));
    }

    void GetAmsConfig(ul::Version &out_ams_version, bool &out_is_emummc) {
        UL_RC_ASSERT(splInitialize());
        // Since we rely on ams for uLaunch to work, it *must* be present
        u64 raw_ams_ver;
        UL_RC_ASSERT(splGetConfig(static_cast<SplConfigItem>(ExosphereApiVersionConfigItem), &raw_ams_ver));
        out_ams_version = {
            .major = static_cast<u8>((raw_ams_ver >> 56) & 0xFF),
            .minor = static_cast<u8>((raw_ams_ver >> 48) & 0xFF),
            .micro = static_cast<u8>((raw_ams_ver >> 40) & 0xFF)
        };
        u64 emummc_type;
        UL_RC_ASSERT(splGetConfig(static_cast<SplConfigItem>(ExosphereEmuMMCType), &emummc_type));
        out_is_emummc = emummc_type != 0;
        splExit();
    }

    bool IsEmuMMC() {
        if(g_GotIsEmuMMC) {
            return g_IsEmuMMC;
        }

        UL_RC_ASSERT(splInitialize());
        u64 emummc_type;
        UL_RC_ASSERT(splGetConfig(static_cast<SplConfigItem>(ExosphereEmuMMCType), &emummc_type));
        g_IsEmuMMC = emummc_type != 0;
        splExit();
        g_GotIsEmuMMC = true;
        
        return g_IsEmuMMC;
    }

}

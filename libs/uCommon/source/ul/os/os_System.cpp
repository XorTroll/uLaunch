#include <ul/os/os_System.hpp>
#include <ctime>

namespace ul::os {

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

        return Time(static_cast<u32>(local_time->tm_hour), static_cast<u32>(local_time->tm_min));
    }

    Date GetCurrentDate() {
        const auto time_val = time(nullptr);
        const auto local_time = localtime(&time_val);

        return Date(static_cast<u32>(local_time->tm_mday), static_cast<u32>(local_time->tm_wday), static_cast<u32>(local_time->tm_mon + 1));
    }

}

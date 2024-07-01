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
    
    void GetCurrentTime(u32 &out_h, u32 &out_min, u32 &out_sec) {
        const auto time_val = time(nullptr);
        const auto local_time = localtime(&time_val);

        out_h = local_time->tm_hour;
        out_min = local_time->tm_min;
        out_sec = local_time->tm_sec;
    }

    std::string GetCurrentDate(const std::vector<std::string> &weekday_list) {
        const auto time_val = time(nullptr);
        const auto local_time = localtime(&time_val);

        char str[0x40] = {};
        sprintf(str, "%02d/%02d (%s)", local_time->tm_mday, local_time->tm_mon + 1, weekday_list.at(local_time->tm_wday).c_str());
        return str;
    }

}

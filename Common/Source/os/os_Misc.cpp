#include <os/os_Misc.hpp>

namespace os
{
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
}
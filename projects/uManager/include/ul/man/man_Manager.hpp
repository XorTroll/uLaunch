
#pragma once

namespace ul::man {

    constexpr const char ActiveSystemPath[] = "sdmc:/atmosphere/contents/0100000000001000";
    constexpr const char BaseSystemPath[] = "sdmc:/ulaunch/bin/uSystem";

    bool IsBasePresent();
    bool IsSystemActive();

    void ActivateSystem();
    void DeactivateSystem();

}
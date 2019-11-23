
#pragma once
#include <q_Include.hpp>

namespace os
{
    enum class GeneralChannelMessage : u32
    {
        Invalid,
        HomeButton = 2,
        Sleep = 3,
        Shutdown = 5,
        Reboot = 6,
        OverlayBrightValueChanged = 13,
        OverlayAutoBrightnessChanged = 14,
        OverlayAirplaneModeChanged = 15,
        HomeButtonHold = 16,
        OverlayHidden = 17,
    };

    struct SystemAppletMessage
    {
        u32 magic;
        u32 unk;
        u32 message;
        u8 data[0x400 - (sizeof(u32) * 3)]; // 1024 bytes are usually sent, so let's read it all.
    } PACKED;

    static constexpr u32 SAMSMagic = 0x534D4153; // "SAMS" -> System Applet message...?

    static_assert(sizeof(SystemAppletMessage) == 0x400, "System applet message must be 0x400!");

    enum class AppletMessage : u32
    {
        Invalid,
        Exit = 4,
        FocusStateChange = 0xF,
        HomeButton = 0x14,
        PowerButton = 22,
        BackFromSleep = 26,
        ChangeOperationMode = 0x1E,
        ChangePerformanceMode = 0x1F,
        SdCardOut = 33,
    };

    Result PushSystemAppletMessage(SystemAppletMessage msg);
}
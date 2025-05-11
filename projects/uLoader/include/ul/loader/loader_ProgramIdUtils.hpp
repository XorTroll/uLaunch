
#pragma once
#include <switch.h>

namespace ul::loader {

    inline constexpr bool IsApplet(const u64 program_id) {
        return (0x0100000000001000 <= program_id) && (program_id <= 0x0100000000001FFF);
    }

    inline constexpr bool IsApplication(const u64 program_id) {
        // TODO (pedantic): shall we also be strict on the upper bound?
        return (0x0100000000010000 <= program_id);
    }

    constexpr AppletType Applet = AppletType_LibraryApplet;
    constexpr AppletType Application = AppletType_SystemApplication;

    inline constexpr AppletType GetAppletType(const u64 program_id) {
        if(IsApplet(program_id)) {
            // OverlayApplet and SystemApplet are impossible in this case
            return Applet;
        }
        else if(IsApplication(program_id)) {
            // hbloader uses this instead of regular Application
            return Application;
        }
        else {
            return AppletType_None;
        }
    }

    void DetermineSelfAppletType(const u64 self_program_id);
    AppletType GetSelfAppletType();
    bool SelfIsApplet();
    bool SelfIsApplication();

}
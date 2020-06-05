#include <am/am_HomeMenu.hpp>
#include <am/am_LibraryApplet.hpp>

namespace am {

    bool g_home_has_focus = true;
    extern AppletHolder g_applet_holder;

    bool HomeMenuHasForeground() {
        return g_home_has_focus;
    }

    Result HomeMenuSetForeground() {
        R_TRY(appletRequestToGetForeground());
        g_home_has_focus = true;
        return ResultSuccess;
    }

}
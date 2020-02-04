#include <am/am_HomeMenu.hpp>
#include <am/am_LibraryApplet.hpp>

namespace am
{
    bool g_home_has_focus = true;
    extern AppletHolder g_applet_holder;

    bool HomeMenuHasForeground()
    {
        return g_home_has_focus;
    }

    Result HomeMenuSetForeground()
    {
        Result rc = appletRequestToGetForeground();
        if(R_SUCCEEDED(rc)) g_home_has_focus = true;
        return rc;
    }
}
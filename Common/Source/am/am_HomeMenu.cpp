#include <am/am_HomeMenu.hpp>
#include <am/am_LibraryApplet.hpp>

namespace am
{
    bool home_focus = true;
    extern AppletHolder applet_holder;

    bool HomeMenuHasForeground()
    {
        return home_focus;
    }

    Result HomeMenuSetForeground()
    {
        Result rc = appletRequestToGetForeground();
        if(R_SUCCEEDED(rc)) home_focus = true;
        return rc;
    }
}
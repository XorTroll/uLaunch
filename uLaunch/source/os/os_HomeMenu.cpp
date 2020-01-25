#include <os/os_HomeMenu.hpp>

namespace os
{
    Result PushSystemAppletMessage(SystemAppletMessage msg)
    {
        AppletStorage st;
        auto rc = appletCreateStorage(&st, sizeof(msg));
        if(R_SUCCEEDED(rc))
        {
            rc = appletStorageWrite(&st, 0, &msg, sizeof(msg));
            if(R_SUCCEEDED(rc)) appletPushToGeneralChannel(&st);
            appletStorageClose(&st);
        }
        return rc;
    }
}
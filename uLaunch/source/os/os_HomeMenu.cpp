#include <os/os_HomeMenu.hpp>

namespace os {

    Result PushSystemAppletMessage(SystemAppletMessage msg) {
        AppletStorage st;
        R_TRY(appletCreateStorage(&st, sizeof(msg)));
        UL_ON_SCOPE_EXIT({
            appletStorageClose(&st);
        });
        R_TRY(appletStorageWrite(&st, 0, &msg, sizeof(msg)));
        R_TRY(appletPushToGeneralChannel(&st));
        return ResultSuccess;
    }
}
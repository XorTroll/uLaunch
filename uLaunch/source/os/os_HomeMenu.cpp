#include <os/os_HomeMenu.hpp>

namespace os {

    Result PushSystemAppletMessage(const SystemAppletMessage msg) {
        AppletStorage st;
        UL_RC_TRY(appletCreateStorage(&st, sizeof(msg)));
        UL_ON_SCOPE_EXIT({ appletStorageClose(&st); });

        UL_RC_TRY(appletStorageWrite(&st, 0, &msg, sizeof(msg)));
        UL_RC_TRY(appletPushToGeneralChannel(&st));
        return ResultSuccess;
    }
}
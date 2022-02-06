#include <am/am_HomeMenu.hpp>

namespace am {

    bool g_DaemonHasFocus = true;

    bool HomeMenuHasForeground() {
        return g_DaemonHasFocus;
    }

    Result HomeMenuSetForeground() {
        UL_RC_TRY(appletRequestToGetForeground());
        g_DaemonHasFocus = true;
        return ResultSuccess;
    }

}
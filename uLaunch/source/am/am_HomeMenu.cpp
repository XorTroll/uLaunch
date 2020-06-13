#include <am/am_HomeMenu.hpp>
#include <am/am_LibraryApplet.hpp>

namespace am {

    bool g_DaemonHasFocus = true;

    bool HomeMenuHasForeground() {
        return g_DaemonHasFocus;
    }

    Result HomeMenuSetForeground() {
        R_TRY(appletRequestToGetForeground());
        g_DaemonHasFocus = true;
        return ResultSuccess;
    }

}
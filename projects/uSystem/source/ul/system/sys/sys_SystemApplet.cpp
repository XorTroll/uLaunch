#include <ul/system/sys/sys_SystemApplet.hpp>
#include <ul/ul_Result.hpp>

namespace ul::system::app {

    extern bool g_ApplicationHasFocus;

}

namespace ul::system::sys {

    bool HasForeground() {
        return !app::g_ApplicationHasFocus;
    }

    Result SetForeground() {
        UL_RC_TRY(appletRequestToGetForeground());

        app::g_ApplicationHasFocus = false;
        return ResultSuccess;
    }

}
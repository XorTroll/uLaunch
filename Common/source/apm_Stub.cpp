#include <q_Include.hpp>

extern "C"
{
    // Due to a weird bug with apm (and thanks that it isn't basically used in libnx) it is stubbed here to avoid any crashes.

    Result __wrap_apmInitialize()
    {
        return 0;
    }

    Result __wrap_apmSetPerformanceConfiguration(ApmPerformanceMode mode, u32 conf)
    {
        return 0;
    }

    void __wrap_apmExit()
    {
    }
}
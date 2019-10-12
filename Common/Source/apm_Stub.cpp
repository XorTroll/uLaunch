#include <q_Include.hpp>

extern "C"
{
    // Due to a weird bug with apm (and thanks that it isn't basically used in libnx) I stub it here to avoid any crashes.

    Result __wrap_apmInitialize()
    {
        return 0;
    }

    Result __wrap_apmSetPerformanceConfiguration(u32 a, u32 b)
    {
        return 0;
    }

    void __wrap_apmExit()
    {
    }
}
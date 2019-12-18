#include <q_Include.hpp>

extern "C"
{
    void __appExit(void);
    void NORETURN __nx_exit(Result rc, LoaderReturnFn retaddr);

    void NORETURN __libnx_exit(int rc)
    {
        // Call destructors.
        void __libc_fini_array(void);
        __libc_fini_array();

        // Clean up services.
        __appExit();

        // 3.0.0 libnx fucked up _appletExitProcess() process-exiting call, and fixed it in later commits.
        // Adding this manually until a new version is released with the fix, then this will be removed.

        // Explicitly initialize applet again, since applet-based exiting needs this, and since it isn't added in 3.0.0, we wrap it here
        // TODO: remove this when next libnx releases
        appletInitialize();

        __nx_exit(0, envGetExitFuncPtr());
    }
}
#include <am/am_LibraryApplet.hpp>

namespace am
{
    AppletHolder applet_holder;
    AppletId applet_lastid;

    bool LibraryAppletIsActive()
    {
        if(applet_holder.StateChangedEvent.revent == INVALID_HANDLE) return false;
        if(!serviceIsActive(&applet_holder.s)) return false;
        return !appletHolderCheckFinished(&applet_holder);
    }

    bool LibraryAppletIsQMenu()
    {
        return (LibraryAppletIsActive() && (applet_lastid == AppletId_shop));
    }

    void LibraryAppletTerminate()
    {
        appletHolderRequestExitOrTerminate(&applet_holder, 10'000'000);
    }

    Result LibraryAppletStart(AppletId id, u32 la_version, void *in_data, size_t in_size)
    {
        if(LibraryAppletIsActive()) LibraryAppletTerminate();
        appletHolderClose(&applet_holder);
        LibAppletArgs largs;
        libappletArgsCreate(&largs, la_version);
        R_TRY(appletCreateLibraryApplet(&applet_holder, id, LibAppletMode_AllForeground));
        R_TRY(libappletArgsPush(&largs, &applet_holder));
        if(in_size > 0)
        {
            R_TRY(LibraryAppletSend(in_data, in_size));
        }
        R_TRY(appletHolderStart(&applet_holder));
        applet_lastid = id;
        return 0;
    }

    Result LibraryAppletSend(void *data, size_t size)
    {
        return libappletPushInData(&applet_holder, data, size);
    }

    Result LibraryAppletRead(void *data, size_t size)
    {
        return libappletPopOutData(&applet_holder, data, size, NULL);
    }
}
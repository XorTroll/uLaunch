#include <am/am_LibraryApplet.hpp>

namespace am
{
    AppletHolder applet_holder;
    AppletId applet_lastid;

    // Grabbed from libnx's source
    static u32 WebApplet_GetLaVersion()
    {
        u32 ver = 0;
        u32 hosver = hosversionGet();
        if(hosver >= MAKEHOSVERSION(8,0,0)) ver = 0x80000;
        else if(hosver >= MAKEHOSVERSION(6,0,0)) ver = 0x60000;
        else if(hosver >= MAKEHOSVERSION(5,0,0)) ver = 0x50000;
        else if(hosver >= MAKEHOSVERSION(3,0,0)) ver = 0x30000;
        else ver = 0x20000;
        return ver;
    }

    bool LibraryAppletIsActive()
    {
        if(applet_holder.StateChangedEvent.revent == INVALID_HANDLE) return false;
        if(!serviceIsActive(&applet_holder.s)) return false;
        return !appletHolderCheckFinished(&applet_holder);
    }

    bool LibraryAppletIsQMenu()
    {
        return (LibraryAppletIsActive() && (LibraryAppletGetId() == QMenuAppletId));
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

    Result WebAppletStart(WebCommonConfig *web)
    {
        return LibraryAppletStart(AppletId_web, WebApplet_GetLaVersion(), &web->arg, sizeof(web->arg));
    }

    AppletId LibraryAppletGetId()
    {
        auto idcopy = applet_lastid;
        if(!LibraryAppletIsActive()) applet_lastid = (AppletId)0; // Invalid
        return idcopy;
    }
}
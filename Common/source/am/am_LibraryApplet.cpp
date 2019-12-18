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
        return LibraryAppletStart(AppletId_web, web->version, &web->arg, sizeof(web->arg));
    }

    Result LibraryAppletQMenuLaunchWith(AppletId id, u32 la_version, std::function<void(AppletHolder*)> on_prepare, std::function<void(AppletHolder*)> on_finish, std::function<bool()> on_wait)
    {
        if(LibraryAppletIsActive()) LibraryAppletTerminate();
        appletHolderClose(&applet_holder);
        LibAppletArgs largs;
        libappletArgsCreate(&largs, la_version);
        R_TRY(appletCreateLibraryApplet(&applet_holder, id, LibAppletMode_AllForeground));
        R_TRY(libappletArgsPush(&largs, &applet_holder));
        on_prepare(&applet_holder);
        R_TRY(appletHolderStart(&applet_holder));
        while(true)
        {
            if(!LibraryAppletIsActive()) break;
            if(!on_wait())
            {
                LibraryAppletTerminate();
                break;
            }
            svcSleepThread(10'000'000);
        }
        on_finish(&applet_holder);
        appletHolderClose(&applet_holder);
        return 0;
    }

    AppletId LibraryAppletGetId()
    {
        auto idcopy = applet_lastid;
        if(!LibraryAppletIsActive()) applet_lastid = (AppletId)0; // Invalid
        return idcopy;
    }
}
#include <am/am_LibraryApplet.hpp>

namespace am
{
    AppletHolder applet_holder;
    AppletId applet_lastid;

    bool LibraryAppletIsActive()
    {
        bool active = false;
        if(applet_holder.StateChangedEvent.revent == INVALID_HANDLE) active = false;
        if(!serviceIsActive(&applet_holder.s)) active = false;
        active = !appletHolderCheckFinished(&applet_holder);
        if(!active) appletUpdateCallerAppletCaptureImage();
        return active;
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

    Result LibraryAppletQMenuLaunchAnd(AppletId id, u32 la_version, void *in_data, size_t in_size, void *out_data, size_t out_size, std::function<bool()> on_wait)
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
        if(out_size > 0) libappletPopOutData(&applet_holder, out_data, out_size, NULL);
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
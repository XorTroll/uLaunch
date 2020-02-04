#include <am/am_LibraryApplet.hpp>

namespace am
{
    static AppletHolder applet_holder;
    static AppletId applet_menuid = InvalidAppletId;
    static AppletId applet_lastid = InvalidAppletId;

    static std::map<u64, AppletId> applet_id_table =
    {
        { 0x0100000000001001, AppletId_auth },
        { 0x0100000000001002, AppletId_cabinet },
        { 0x0100000000001003, AppletId_controller },
        { 0x0100000000001004, AppletId_dataErase },
        { 0x0100000000001005, AppletId_error },
        { 0x0100000000001006, AppletId_netConnect },
        { 0x0100000000001007, AppletId_playerSelect },
        { 0x0100000000001008, AppletId_swkbd },
        { 0x0100000000001009, AppletId_miiEdit },
        { 0x010000000000100A, AppletId_web },
        { 0x010000000000100B, AppletId_shop },
        { 0x010000000000100D, AppletId_photoViewer },
        { 0x010000000000100E, AppletId_set },
        { 0x010000000000100F, AppletId_offlineWeb },
        { 0x0100000000001010, AppletId_loginShare },
        { 0x0100000000001011, AppletId_wifiWebAuth },
        { 0x0100000000001013, AppletId_myPage }
    };

    bool LibraryAppletIsActive()
    {
        if(applet_holder.StateChangedEvent.revent == INVALID_HANDLE) return false;
        if(!serviceIsActive(&applet_holder.s)) return false;
        return !appletHolderCheckFinished(&applet_holder);
    }

    void LibraryAppletSetMenuAppletId(AppletId id)
    {
        applet_menuid = id;
    }

    bool LibraryAppletIsMenu()
    {
        return (LibraryAppletIsActive() && (applet_menuid != InvalidAppletId) && (LibraryAppletGetId() == applet_menuid));
    }

    void LibraryAppletTerminate()
    {
        // Give it 15 seconds
        appletHolderRequestExitOrTerminate(&applet_holder, 15'000'000'000ul);
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

    Result LibraryAppletDaemonLaunchWith(AppletId id, u32 la_version, std::function<void(AppletHolder*)> on_prepare, std::function<void(AppletHolder*)> on_finish, std::function<bool()> on_wait)
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
            svcSleepThread(10'000'000l);
        }
        on_finish(&applet_holder);
        appletHolderClose(&applet_holder);
        return 0;
    }

    u64 LibraryAppletGetProgramIdForAppletId(AppletId id)
    {
        for(auto &[program_id, applet_id] : applet_id_table)
        {
            if(applet_id == id) return program_id;
        }
        return 0;
    }

    AppletId LibraryAppletGetAppletIdForProgramId(u64 id)
    {
        for(auto &[program_id, applet_id] : applet_id_table)
        {
            if(program_id == id) return applet_id;
        }
        return InvalidAppletId;
    }

    AppletId LibraryAppletGetId()
    {
        auto idcopy = applet_lastid;
        if(!LibraryAppletIsActive()) applet_lastid = InvalidAppletId;
        return idcopy;
    }
}
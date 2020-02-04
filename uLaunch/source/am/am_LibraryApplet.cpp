#include <am/am_LibraryApplet.hpp>

namespace am
{
    static AppletHolder g_applet_holder;
    static AppletId g_menu_applet_id = InvalidAppletId;
    static AppletId g_last_applet_id = InvalidAppletId;

    static std::map<u64, AppletId> g_applet_id_table =
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
        if(g_applet_holder.StateChangedEvent.revent == INVALID_HANDLE) return false;
        if(!serviceIsActive(&g_applet_holder.s)) return false;
        return !appletHolderCheckFinished(&g_applet_holder);
    }

    void LibraryAppletSetMenuAppletId(AppletId id)
    {
        g_menu_applet_id = id;
    }

    bool LibraryAppletIsMenu()
    {
        return (LibraryAppletIsActive() && (g_menu_applet_id != InvalidAppletId) && (LibraryAppletGetId() == g_menu_applet_id));
    }

    void LibraryAppletTerminate()
    {
        // Give it 15 seconds
        appletHolderRequestExitOrTerminate(&g_applet_holder, 15'000'000'000ul);
    }

    Result LibraryAppletStart(AppletId id, u32 la_version, void *in_data, size_t in_size)
    {
        if(LibraryAppletIsActive()) LibraryAppletTerminate();
        appletHolderClose(&g_applet_holder);
        LibAppletArgs largs;
        libappletArgsCreate(&largs, la_version);
        R_TRY(appletCreateLibraryApplet(&g_applet_holder, id, LibAppletMode_AllForeground));
        R_TRY(libappletArgsPush(&largs, &g_applet_holder));
        if(in_size > 0)
        {
            R_TRY(LibraryAppletSend(in_data, in_size));
        }
        R_TRY(appletHolderStart(&g_applet_holder));
        g_last_applet_id = id;
        return 0;
    }

    Result LibraryAppletSend(void *data, size_t size)
    {
        return libappletPushInData(&g_applet_holder, data, size);
    }

    Result LibraryAppletRead(void *data, size_t size)
    {
        return libappletPopOutData(&g_applet_holder, data, size, NULL);
    }

    Result WebAppletStart(WebCommonConfig *web)
    {
        return LibraryAppletStart(AppletId_web, web->version, &web->arg, sizeof(web->arg));
    }

    Result LibraryAppletDaemonLaunchWith(AppletId id, u32 la_version, std::function<void(AppletHolder*)> on_prepare, std::function<void(AppletHolder*)> on_finish, std::function<bool()> on_wait)
    {
        if(LibraryAppletIsActive()) LibraryAppletTerminate();
        appletHolderClose(&g_applet_holder);
        LibAppletArgs largs;
        libappletArgsCreate(&largs, la_version);
        R_TRY(appletCreateLibraryApplet(&g_applet_holder, id, LibAppletMode_AllForeground));
        R_TRY(libappletArgsPush(&largs, &g_applet_holder));
        on_prepare(&g_applet_holder);
        R_TRY(appletHolderStart(&g_applet_holder));
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
        on_finish(&g_applet_holder);
        appletHolderClose(&g_applet_holder);
        return 0;
    }

    u64 LibraryAppletGetProgramIdForAppletId(AppletId id)
    {
        for(auto &[program_id, applet_id] : g_applet_id_table)
        {
            if(applet_id == id) return program_id;
        }
        return 0;
    }

    AppletId LibraryAppletGetAppletIdForProgramId(u64 id)
    {
        for(auto &[program_id, applet_id] : g_applet_id_table)
        {
            if(program_id == id) return applet_id;
        }
        return InvalidAppletId;
    }

    AppletId LibraryAppletGetId()
    {
        auto idcopy = g_last_applet_id;
        if(!LibraryAppletIsActive()) g_last_applet_id = InvalidAppletId;
        return idcopy;
    }
}
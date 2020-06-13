#include <am/am_LibraryApplet.hpp>

namespace am {

    namespace {

        AppletHolder g_AppletHolder;
        AppletId g_MenuAppletId = InvalidAppletId;
        AppletId g_LastAppletId = InvalidAppletId;

        const std::map<u64, AppletId> g_AppletIdTable = {
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

    }

    bool LibraryAppletIsActive() {
        if(g_AppletHolder.StateChangedEvent.revent == INVALID_HANDLE) {
            return false;
        }
        if(!serviceIsActive(&g_AppletHolder.s)) {
            return false;
        }
        return !appletHolderCheckFinished(&g_AppletHolder);
    }

    void LibraryAppletSetMenuAppletId(AppletId id) {
        g_MenuAppletId = id;
    }

    AppletId LibraryAppletGetMenuAppletId() {
        return g_MenuAppletId;
    }

    bool LibraryAppletIsMenu() {
        return LibraryAppletIsActive() && (g_MenuAppletId != InvalidAppletId) && (LibraryAppletGetId() == g_MenuAppletId);
    }

    void LibraryAppletTerminate() {
        // Give it 15 seconds
        appletHolderRequestExitOrTerminate(&g_AppletHolder, 15'000'000'000ul);
    }

    Result LibraryAppletStart(AppletId id, u32 la_version, void *in_data, size_t in_size) {
        if(LibraryAppletIsActive()) {
            LibraryAppletTerminate();
        }
        appletHolderClose(&g_AppletHolder);
        LibAppletArgs largs;
        libappletArgsCreate(&largs, la_version);
        R_TRY(appletCreateLibraryApplet(&g_AppletHolder, id, LibAppletMode_AllForeground));
        R_TRY(libappletArgsPush(&largs, &g_AppletHolder));
        if(in_size > 0) {
            R_TRY(LibraryAppletSend(in_data, in_size));
        }
        R_TRY(appletHolderStart(&g_AppletHolder));
        g_LastAppletId = id;
        return ResultSuccess;
    }

    Result LibraryAppletSend(void *data, size_t size) {
        return libappletPushInData(&g_AppletHolder, data, size);
    }

    Result LibraryAppletRead(void *data, size_t size) {
        return libappletPopOutData(&g_AppletHolder, data, size, nullptr);
    }

    Result LibraryAppletDaemonLaunchWith(AppletId id, u32 la_version, std::function<void(AppletHolder*)> on_prepare, std::function<void(AppletHolder*)> on_finish, std::function<bool()> on_wait) {
        if(LibraryAppletIsActive()) {
            LibraryAppletTerminate();
        }
        appletHolderClose(&g_AppletHolder);
        LibAppletArgs largs;
        libappletArgsCreate(&largs, la_version);
        R_TRY(appletCreateLibraryApplet(&g_AppletHolder, id, LibAppletMode_AllForeground));
        R_TRY(libappletArgsPush(&largs, &g_AppletHolder));
        on_prepare(&g_AppletHolder);
        R_TRY(appletHolderStart(&g_AppletHolder));
        while(true) {
            if(!LibraryAppletIsActive()) {
                break;
            }
            if(!on_wait()) {
                LibraryAppletTerminate();
                break;
            }
            svcSleepThread(10'000'000ul);
        }
        on_finish(&g_AppletHolder);
        appletHolderClose(&g_AppletHolder);
        return ResultSuccess;
    }

    u64 LibraryAppletGetProgramIdForAppletId(AppletId id) {
        for(auto &[program_id, applet_id] : g_AppletIdTable) {
            if(applet_id == id) {
                return program_id;
            }
        }
        return 0;
    }

    AppletId LibraryAppletGetAppletIdForProgramId(u64 id) {
        auto it = g_AppletIdTable.find(id);
        if(it != g_AppletIdTable.end()) {
            return it->second;
        }
        return InvalidAppletId;
    }

    AppletId LibraryAppletGetId() {
        auto idcopy = g_LastAppletId;
        if(!LibraryAppletIsActive()) {
            g_LastAppletId = InvalidAppletId;
        }
        return idcopy;
    }
}
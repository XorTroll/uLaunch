#include <ul/system/la/la_LibraryApplet.hpp>
#include <ul/ul_Result.hpp>

namespace ul::system::la {

    namespace {

        AppletHolder g_LibraryAppletHolder;
        AppletId g_MenuAppletId = AppletId_None;
        AppletId g_LastAppletId = AppletId_None;

        struct LibraryAppletInfo {
            u64 program_id;
            AppletId applet_id;
        };

        constexpr LibraryAppletInfo g_LibraryAppletTable[] = {
            { 0x0100000000001001, AppletId_LibraryAppletAuth },
            { 0x0100000000001002, AppletId_LibraryAppletCabinet },
            { 0x0100000000001003, AppletId_LibraryAppletController },
            { 0x0100000000001004, AppletId_LibraryAppletDataErase },
            { 0x0100000000001005, AppletId_LibraryAppletError },
            { 0x0100000000001006, AppletId_LibraryAppletNetConnect },
            { 0x0100000000001007, AppletId_LibraryAppletPlayerSelect },
            { 0x0100000000001008, AppletId_LibraryAppletSwkbd },
            { 0x0100000000001009, AppletId_LibraryAppletMiiEdit },
            { 0x010000000000100A, AppletId_LibraryAppletWeb },
            { 0x010000000000100B, AppletId_LibraryAppletShop },
            { 0x010000000000100D, AppletId_LibraryAppletPhotoViewer },
            { 0x010000000000100E, AppletId_LibraryAppletSet },
            { 0x010000000000100F, AppletId_LibraryAppletOfflineWeb },
            { 0x0100000000001010, AppletId_LibraryAppletLoginShare },
            { 0x0100000000001011, AppletId_LibraryAppletWifiWebAuth },
            { 0x0100000000001013, AppletId_LibraryAppletMyPage }
        };
        constexpr size_t LibraryAppletCount = sizeof(g_LibraryAppletTable) / sizeof(LibraryAppletInfo);

        Result Create(const AppletId id, const s32 la_version) {
            if(IsActive()) {
                UL_RC_TRY(Terminate());
            }
        
            UL_RC_TRY(appletCreateLibraryApplet(&g_LibraryAppletHolder, id, LibAppletMode_AllForeground));
    
            // Treat -1/any negative pseudovalue as to not push these args
            if(la_version >= 0) {
                LibAppletArgs la_args;
                libappletArgsCreate(&la_args, (u32)la_version);
                // TODO (low priority): does this make any difference?
                libappletArgsSetPlayStartupSound(&la_args, true);
                UL_RC_TRY(libappletArgsPush(&la_args, &g_LibraryAppletHolder));
            }
    
            return ResultSuccess;
        }

        Result Launch(const AppletId created_id) {
            UL_RC_TRY(appletHolderStart(&g_LibraryAppletHolder));
            g_LastAppletId = created_id;
            return ResultSuccess;
        }

    }

    bool IsActive() {
        if(g_LibraryAppletHolder.StateChangedEvent.revent == INVALID_HANDLE) {
            return false;
        }
        if(!serviceIsActive(&g_LibraryAppletHolder.s)) {
            return false;
        }
        return !appletHolderCheckFinished(&g_LibraryAppletHolder);
    }

    Result Terminate() {
        // Give it 15 seconds
        UL_RC_TRY(appletHolderRequestExitOrTerminate(&g_LibraryAppletHolder, 15'000'000'000ul));
        appletHolderClose(&g_LibraryAppletHolder);

        UL_RC_SUCCEED;
    }

    Result Start(const AppletId id, const s32 la_version) {
        UL_RC_TRY(Create(id, la_version));
        UL_RC_TRY(Launch(id));
        return ResultSuccess;
    }
    
    Result Start(const AppletId id, const s32 la_version, const void *in_data, const size_t in_size) {
        UL_RC_TRY(Create(id, la_version));

        if((in_data != nullptr) && (in_size > 0)) {
            UL_RC_TRY(Send(in_data, in_size));
        }

        UL_RC_TRY(Launch(id));
        return ResultSuccess;
    }

    Result Start(const AppletId id, const s32 la_version, const void *in_data, const size_t in_size, const void *in_data_2, const size_t in_size_2) {
        UL_RC_TRY(Create(id, la_version));

        if((in_data != nullptr) && (in_size > 0)) {
            UL_RC_TRY(Send(in_data, in_size));
        }
        if((in_data_2 != nullptr) && (in_size_2 > 0)) {
            UL_RC_TRY(Send(in_data_2, in_size_2));
        }

        UL_RC_TRY(Launch(id));
        return ResultSuccess;
    }

    Result Send(const void *data, const size_t size) {
        return libappletPushInData(&g_LibraryAppletHolder, data, size);
    }

    Result Read(void *data, const size_t size) {
        return libappletPopOutData(&g_LibraryAppletHolder, data, size, nullptr);
    }

    Result Push(AppletStorage *st) {
        return appletHolderPushInData(&g_LibraryAppletHolder, st);
    }

    Result Pop(AppletStorage *st) {
        return appletHolderPopOutData(&g_LibraryAppletHolder, st);
    }

    u64 GetProgramIdForAppletId(const AppletId id) {
        for(u32 i = 0; i < LibraryAppletCount; i++) {
            const auto info = g_LibraryAppletTable[i];
            if(info.applet_id == id) {
                return info.program_id;
            }
        }

        return 0;
    }

    AppletId GetAppletIdForProgramId(const u64 id) {
        for(u32 i = 0; i < LibraryAppletCount; i++) {
            const auto info = g_LibraryAppletTable[i];
            if(info.program_id == id) {
                return info.applet_id;
            }
        }

        return AppletId_None;
    }

    AppletId GetLastAppletId() {
        auto last_id_copy = g_LastAppletId;
        if(!IsActive()) {
            g_LastAppletId = AppletId_None;
        }
        return last_id_copy;
    }

    void SetMenuAppletId(const AppletId id) {
        g_MenuAppletId = id;
    }

    AppletId GetMenuAppletId() {
        return g_MenuAppletId;
    }

}

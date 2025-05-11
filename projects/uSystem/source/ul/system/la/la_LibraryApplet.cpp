#include <ul/system/la/la_LibraryApplet.hpp>
#include <ul/la/la_LibraryApplets.hpp>
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

        #define _UL_LA_TABLE_ENTRY(applet) LibraryAppletInfo { ::ul::la::applet, AppletId_##applet }

        constexpr LibraryAppletInfo g_LibraryAppletTable[] = {
            _UL_LA_TABLE_ENTRY(LibraryAppletAuth),
            _UL_LA_TABLE_ENTRY(LibraryAppletCabinet),
            _UL_LA_TABLE_ENTRY(LibraryAppletController),
            _UL_LA_TABLE_ENTRY(LibraryAppletDataErase),
            _UL_LA_TABLE_ENTRY(LibraryAppletError),
            _UL_LA_TABLE_ENTRY(LibraryAppletNetConnect),
            _UL_LA_TABLE_ENTRY(LibraryAppletPlayerSelect),
            _UL_LA_TABLE_ENTRY(LibraryAppletSwkbd),
            _UL_LA_TABLE_ENTRY(LibraryAppletMiiEdit),
            _UL_LA_TABLE_ENTRY(LibraryAppletWeb),
            _UL_LA_TABLE_ENTRY(LibraryAppletShop),
            _UL_LA_TABLE_ENTRY(LibraryAppletPhotoViewer),
            _UL_LA_TABLE_ENTRY(LibraryAppletSet),
            _UL_LA_TABLE_ENTRY(LibraryAppletOfflineWeb),
            _UL_LA_TABLE_ENTRY(LibraryAppletLoginShare),
            _UL_LA_TABLE_ENTRY(LibraryAppletWifiWebAuth),
            _UL_LA_TABLE_ENTRY(LibraryAppletMyPage),
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

        UL_ASSERT_FAIL("Invalid applet ID: 0x%08X", id);
    }

    AppletId GetAppletIdForProgramId(const u64 id) {
        for(u32 i = 0; i < LibraryAppletCount; i++) {
            const auto info = g_LibraryAppletTable[i];
            if(info.program_id == id) {
                return info.applet_id;
            }
        }

        UL_ASSERT_FAIL("Invalid applet program ID: %016lX", id);
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

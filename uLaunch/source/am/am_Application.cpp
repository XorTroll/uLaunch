#include <am/am_Application.hpp>

namespace am {

    namespace {

        AppletApplication g_ApplicationHolder;
        u64 g_LastApplicationId;

    }

    extern bool g_DaemonHasFocus;

    bool ApplicationIsActive() {
        if(!eventActive(&g_ApplicationHolder.StateChangedEvent)) {
            return false;
        }
        if(!serviceIsActive(&g_ApplicationHolder.s)) {
            return false;
        }

        return !appletApplicationCheckFinished(&g_ApplicationHolder);
    }

    void ApplicationTerminate() {
        appletApplicationRequestExit(&g_ApplicationHolder);

        // Wait until it's actually exited
        while(ApplicationIsActive()) {
            svcSleepThread(10'000'000);
        }

        g_DaemonHasFocus = true;
    }

    Result ApplicationStart(const u64 app_id, const bool system, const AccountUid user_id, const void *data, const size_t size) {
        appletApplicationClose(&g_ApplicationHolder);

        if(system) {
            UL_RC_TRY(appletCreateSystemApplication(&g_ApplicationHolder, app_id));
        }
        else {
            // Ensure it's launchable
            UL_RC_TRY(nsTouchApplication(app_id));

            auto ct_data = new NsApplicationControlData;
            UL_ON_SCOPE_EXIT({ delete[] ct_data; });

            size_t dummy_size;
            UL_RC_TRY(nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, ct_data, sizeof(NsApplicationControlData), &dummy_size));

            // Note: why isn't TemporaryStorage automatically created with nsTouchApplication like regular savedata?
            // Let's create it ourselves if it doesn't exist yet
            if(ct_data->nacp.temporary_storage_size > 0) {
                const FsSaveDataAttribute attr = {
                    .application_id = app_id,
                    .system_save_data_id = 0,
                    .save_data_type = FsSaveDataType_Temporary,
                    .save_data_rank = FsSaveDataRank_Primary,
                    .save_data_index = 0
                };
                constexpr auto space_id = FsSaveDataSpaceId_Temporary;
                const FsSaveDataCreationInfo cr_info = {
                    .save_data_size = (s64)ct_data->nacp.temporary_storage_size,
                    .journal_size = 0,
                    .available_size = 0x4000,
                    .owner_id = app_id,
                    .flags = 0,
                    .save_data_space_id = space_id
                };
                const FsSaveDataMetaInfo meta_info = {
                    .size = 0,
                    .type = FsSaveDataMetaType_None
                };

                FsFileSystem dummy_fs;
                if(R_SUCCEEDED(fsOpenSaveDataFileSystem(&dummy_fs, space_id, &attr))) {
                    fsFsClose(&dummy_fs);
                }
                else {
                    // Not yet created, create it then
                    UL_RC_TRY(fsCreateSaveDataFileSystem(&attr, &cr_info, &meta_info));
                }
            }

            UL_RC_TRY(appletCreateApplication(&g_ApplicationHolder, app_id));
        }
        
        if(accountUidIsValid(&user_id)) {
            const auto selected_user_arg = ApplicationSelectedUserArgument::Create(user_id);
            UL_RC_TRY(ApplicationSend(&selected_user_arg, sizeof(selected_user_arg), AppletLaunchParameterKind_PreselectedUser));
        }

        if(size > 0) {
            UL_RC_TRY(ApplicationSend(data, size));
        }

        UL_RC_TRY(appletUnlockForeground());
        UL_RC_TRY(appletApplicationStart(&g_ApplicationHolder));
        UL_RC_TRY(ApplicationSetForeground());

        g_LastApplicationId = app_id;
        return 0;
    }

    bool ApplicationHasForeground() {
        return !g_DaemonHasFocus;
    }

    Result ApplicationSetForeground() {
        UL_RC_TRY(appletApplicationRequestForApplicationToGetForeground(&g_ApplicationHolder));
        g_DaemonHasFocus = false;
        return ResultSuccess;
    }

    Result ApplicationSend(const void *data, const size_t size, const AppletLaunchParameterKind kind) {
        AppletStorage st;
        UL_RC_TRY(appletCreateStorage(&st, size));
        UL_ON_SCOPE_EXIT({ appletStorageClose(&st); });

        UL_RC_TRY(appletStorageWrite(&st, 0, data, size));
        UL_RC_TRY(appletApplicationPushLaunchParameter(&g_ApplicationHolder, kind, &st));
        return ResultSuccess;
    }

    u64 ApplicationGetId() {
        return g_LastApplicationId;
    }

    bool ApplicationNeedsUser(const u64 app_id) {
        auto control_data = new NsApplicationControlData();
        nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, control_data, sizeof(NsApplicationControlData), nullptr);

        const auto needs_user = control_data->nacp.startup_user_account > 0;
        delete control_data;
        return needs_user;
    }
}
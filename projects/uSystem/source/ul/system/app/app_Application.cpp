#include <ul/system/app/app_Application.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Scope.hpp>

namespace ul::system::app {

    namespace {

        AppletApplication g_ApplicationHolder;
        u64 g_LastApplicationId;

    }

    bool g_ApplicationHasFocus;

    bool IsActive() {
        if(!eventActive(&g_ApplicationHolder.StateChangedEvent)) {
            return false;
        }
        if(!serviceIsActive(&g_ApplicationHolder.s)) {
            return false;
        }

        return !appletApplicationCheckFinished(&g_ApplicationHolder);
    }

    Result Terminate() {
        UL_RC_TRY(appletApplicationRequestExit(&g_ApplicationHolder));

        const auto rc = eventWait(&g_ApplicationHolder.StateChangedEvent, 15'000'000'000ul);
        if(rc == KERNELRESULT(TimedOut)) {
            UL_RC_TRY(appletApplicationTerminate(&g_ApplicationHolder));
        }

        const auto app_rc = serviceDispatch(&g_ApplicationHolder.s, 30);
        UL_LOG_WARN("Application terminated with result 0x%X", app_rc);

        appletApplicationClose(&g_ApplicationHolder);
        g_ApplicationHasFocus = false;
        return rc;
    }

    Result Start(const u64 app_id, const bool system, const AccountUid user_id, const void *data, const size_t size) {
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
            UL_RC_TRY(Send(&selected_user_arg, sizeof(selected_user_arg), AppletLaunchParameterKind_PreselectedUser));
        }

        if(size > 0) {
            UL_RC_TRY(Send(data, size));
        }

        UL_RC_TRY(appletUnlockForeground());
        UL_RC_TRY(appletApplicationStart(&g_ApplicationHolder));
        UL_RC_TRY(SetForeground());

        g_LastApplicationId = app_id;
        return ResultSuccess;
    }

    bool HasForeground() {
        return g_ApplicationHasFocus;
    }

    Result SetForeground() {
        UL_RC_TRY(appletApplicationRequestForApplicationToGetForeground(&g_ApplicationHolder));
        g_ApplicationHasFocus = true;
        return ResultSuccess;
    }

    Result Send(const void *data, const size_t size, const AppletLaunchParameterKind kind) {
        AppletStorage st;
        UL_RC_TRY(appletCreateStorage(&st, size));
        util::OnScopeExit st_close([&]() {
            appletStorageClose(&st);
        });

        UL_RC_TRY(appletStorageWrite(&st, 0, data, size));
        UL_RC_TRY(appletApplicationPushLaunchParameter(&g_ApplicationHolder, kind, &st));
        return ResultSuccess;
    }

    u64 GetId() {
        return g_LastApplicationId;
    }

}

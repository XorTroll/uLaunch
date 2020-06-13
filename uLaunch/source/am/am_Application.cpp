#include <am/am_Application.hpp>

namespace am {

    namespace {

        AppletApplication g_ApplicationHolder;
        u64 g_LastApplicationId;

    }

    extern bool g_DaemonHasFocus;

    bool ApplicationIsActive() {
        if(g_ApplicationHolder.StateChangedEvent.revent == INVALID_HANDLE) {
            return false;
        }
        if(!serviceIsActive(&g_ApplicationHolder.s)) {
            return false;
        }
        return !appletApplicationCheckFinished(&g_ApplicationHolder);
    }

    void ApplicationTerminate() {
        appletApplicationRequestExit(&g_ApplicationHolder);
        g_DaemonHasFocus = true;
    }

    Result ApplicationStart(u64 app_id, bool system, AccountUid user_id, void *data, size_t size) {
        appletApplicationClose(&g_ApplicationHolder);
        if(system) {
            R_TRY(appletCreateSystemApplication(&g_ApplicationHolder, app_id));
        }
        else {
            R_TRY(appletCreateApplication(&g_ApplicationHolder, app_id));
        }
        
        if(accountUidIsValid(&user_id)) {
            auto selected_user_arg = ApplicationSelectedUserArgument::Create(user_id);
            R_TRY(ApplicationSend(&selected_user_arg, sizeof(selected_user_arg), AppletLaunchParameterKind_PreselectedUser));
        }

        if(size > 0) {
            R_TRY(ApplicationSend(data, size));
        }

        R_TRY(appletUnlockForeground());
        R_TRY(appletApplicationStart(&g_ApplicationHolder));
        R_TRY(ApplicationSetForeground());

        g_LastApplicationId = app_id;
        return 0;
    }

    bool ApplicationHasForeground() {
        return !g_DaemonHasFocus;
    }

    Result ApplicationSetForeground() {
        R_TRY(appletApplicationRequestForApplicationToGetForeground(&g_ApplicationHolder));
        g_DaemonHasFocus = false;
        return ResultSuccess;
    }

    Result ApplicationSend(void *data, size_t size, AppletLaunchParameterKind kind) {
        AppletStorage st;
        R_TRY(appletCreateStorage(&st, size));
        UL_ON_SCOPE_EXIT({
            appletStorageClose(&st);
        });
        R_TRY(appletStorageWrite(&st, 0, data, size));
        R_TRY(appletApplicationPushLaunchParameter(&g_ApplicationHolder, kind, &st));
        return ResultSuccess;
    }

    u64 ApplicationGetId() {
        return g_LastApplicationId;
    }

    bool ApplicationNeedsUser(u64 app_id) {
        NsApplicationControlData ctdata = {};
        nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, &ctdata, sizeof(ctdata), nullptr);
        return ctdata.nacp.startup_user_account > 0;
    }
}
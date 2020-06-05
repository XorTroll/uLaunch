#include <am/am_Application.hpp>

namespace am
{
    extern bool g_home_has_focus;
    static AppletApplication g_application_holder;
    static u64 g_last_app_id;

    bool ApplicationIsActive() {
        if(g_application_holder.StateChangedEvent.revent == INVALID_HANDLE) {
            return false;
        }
        if(!serviceIsActive(&g_application_holder.s)) {
            return false;
        }
        return !appletApplicationCheckFinished(&g_application_holder);
    }

    void ApplicationTerminate() {
        appletApplicationRequestExit(&g_application_holder);
        g_home_has_focus = true;
    }

    Result ApplicationStart(u64 app_id, bool system, AccountUid user_id, void *data, size_t size) {
        appletApplicationClose(&g_application_holder);
        if(system) {
            R_TRY(appletCreateSystemApplication(&g_application_holder, app_id));
        }
        else {
            R_TRY(appletCreateApplication(&g_application_holder, app_id));
        }
        
        if(accountUidIsValid(&user_id)) {
            auto selected_user_arg = ApplicationSelectedUserArgument::Create(user_id);
            R_TRY(ApplicationSend(&selected_user_arg, sizeof(selected_user_arg), AppletLaunchParameterKind_PreselectedUser));
        }

        if(size > 0) {
            R_TRY(ApplicationSend(data, size));
        }

        R_TRY(appletUnlockForeground());
        R_TRY(appletApplicationStart(&g_application_holder));
        R_TRY(ApplicationSetForeground());

        g_last_app_id = app_id;
        return 0;
    }

    bool ApplicationHasForeground() {
        return !g_home_has_focus;
    }

    Result ApplicationSetForeground() {
        R_TRY(appletApplicationRequestForApplicationToGetForeground(&g_application_holder));
        g_home_has_focus = false;
        return ResultSuccess;
    }

    Result ApplicationSend(void *data, size_t size, AppletLaunchParameterKind kind) {
        AppletStorage st;
        R_TRY(appletCreateStorage(&st, size));
        UL_ON_SCOPE_EXIT({
            appletStorageClose(&st);
        });
        R_TRY(appletStorageWrite(&st, 0, data, size));
        R_TRY(appletApplicationPushLaunchParameter(&g_application_holder, kind, &st));
        return ResultSuccess;
    }

    u64 ApplicationGetId() {
        return g_last_app_id;
    }

    bool ApplicationNeedsUser(u64 app_id) {
        NsApplicationControlData ctdata = {};
        nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, &ctdata, sizeof(ctdata), nullptr);
        return ctdata.nacp.startup_user_account > 0;
    }
}
#include <am/am_Application.hpp>

namespace am
{
    extern bool g_home_has_focus;
    static AppletApplication g_application_holder;
    static u64 g_last_app_id;

    bool ApplicationIsActive()
    {
        if(g_application_holder.StateChangedEvent.revent == INVALID_HANDLE) return false;
        if(!serviceIsActive(&g_application_holder.s)) return false;
        return !appletApplicationCheckFinished(&g_application_holder);
    }

    void ApplicationTerminate()
    {
        appletApplicationRequestExit(&g_application_holder);
        if(!g_home_has_focus) g_home_has_focus = true;
    }

    Result ApplicationStart(u64 app_id, bool system, AccountUid user_id, void *data, size_t size)
    {
        appletApplicationClose(&g_application_holder);
        if(system) R_TRY(appletCreateSystemApplication(&g_application_holder, app_id));
        else R_TRY(appletCreateApplication(&g_application_holder, app_id));
        
        if(accountUidIsValid(&user_id))
        {
            ApplicationSelectedUserArgument arg = {};
            arg.magic = SelectedUserMagic;
            arg.one = 1;
            memcpy(&arg.uid, &user_id, sizeof(user_id));
            ApplicationSend(&arg, sizeof(arg), AppletLaunchParameterKind_PreselectedUser);
        }

        if(size > 0) ApplicationSend(data, size);

        R_TRY(appletUnlockForeground());
        R_TRY(appletApplicationStart(&g_application_holder));
        R_TRY(ApplicationSetForeground());

        g_last_app_id = app_id;

        return 0;
    }

    bool ApplicationHasForeground()
    {
        return !g_home_has_focus;
    }

    Result ApplicationSetForeground()
    {
        auto rc = appletApplicationRequestForApplicationToGetForeground(&g_application_holder);
        if(R_SUCCEEDED(rc)) g_home_has_focus = false;
        return rc;
    }

    Result ApplicationSend(void *data, size_t size, AppletLaunchParameterKind kind)
    {
        AppletStorage st;
        auto rc = appletCreateStorage(&st, size);
        if(R_SUCCEEDED(rc))
        {
            rc = appletStorageWrite(&st, 0, data, size);
            if(R_SUCCEEDED(rc)) rc = appletApplicationPushLaunchParameter(&g_application_holder, kind, &st);
            appletStorageClose(&st);
        }
        return rc;
    }

    u64 ApplicationGetId()
    {
        return g_last_app_id;
    }

    bool ApplicationNeedsUser(u64 app_id)
    {
        NsApplicationControlData ctdata = {};
        nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, &ctdata, sizeof(ctdata), NULL);
        return (ctdata.nacp.startup_user_account > 0);
    }
}
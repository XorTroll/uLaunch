#include <am/am_Application.hpp>

namespace am
{
    extern bool home_focus;
    AppletApplication app_holder;
    u64 latest_appid;

    bool ApplicationIsActive()
    {
        if(app_holder.StateChangedEvent.revent == INVALID_HANDLE) return false;
        if(!serviceIsActive(&app_holder.s)) return false;
        return !appletApplicationCheckFinished(&app_holder);
    }

    void ApplicationTerminate()
    {
        appletApplicationRequestExit(&app_holder);
        if(!home_focus) home_focus = true;
    }

    Result ApplicationStart(u64 app_id, bool system, u128 user_id, void *data, size_t size)
    {
        appletApplicationClose(&app_holder);
        if(system) R_TRY(appletCreateSystemApplication(&app_holder, app_id));
        else R_TRY(appletCreateApplication(&app_holder, app_id));
        
        if(user_id > 0)
        {
            ApplicationSelectedUserArgument arg = {};
            arg.magic = SelectedUserMagic;
            arg.one = 1;
            arg.uid = user_id;
            ApplicationSend(&arg, sizeof(arg), AppletLaunchParameterKind_PreselectedUser);
        }

        if(size > 0) ApplicationSend(data, size);

        R_TRY(appletUnlockForeground());
        R_TRY(appletApplicationStart(&app_holder));
        R_TRY(ApplicationSetForeground());

        latest_appid = app_id;

        return 0;
    }

    bool ApplicationHasForeground()
    {
        return !home_focus;
    }

    Result ApplicationSetForeground()
    {
        auto rc = appletApplicationRequestForApplicationToGetForeground(&app_holder);
        if(R_SUCCEEDED(rc)) home_focus = false;
        return rc;
    }

    Result ApplicationSend(void *data, size_t size, AppletLaunchParameterKind kind)
    {
        AppletStorage st;
        auto rc = appletCreateStorage(&st, size);
        if(R_SUCCEEDED(rc))
        {
            rc = appletStorageWrite(&st, 0, data, size);
            if(R_SUCCEEDED(rc)) rc = appletApplicationPushLaunchParameter(&app_holder, kind, &st);
            appletStorageClose(&st);
        }
        return rc;
    }

    u64 ApplicationGetId()
    {
        return latest_appid;
    }
}
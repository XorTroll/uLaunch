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

    void Terminate() {
        appletApplicationRequestExit(&g_ApplicationHolder);

        // Wait until it's actually exited
        while(IsActive()) {
            svcSleepThread(10'000'000);
        }

        g_ApplicationHasFocus = false;
    }

    Result Start(const u64 app_id, const bool system, const AccountUid user_id, const void *data, const size_t size) {
        appletApplicationClose(&g_ApplicationHolder);

        if(system) {
            UL_RC_TRY(appletCreateSystemApplication(&g_ApplicationHolder, app_id));
        }
        else {
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
        return 0;
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
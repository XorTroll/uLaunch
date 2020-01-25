
#pragma once
#include <ul_Include.hpp>
#include <os/os_Titles.hpp>

namespace am
{
    struct ApplicationSelectedUserArgument
    {
        u32 magic;
        u8 one;
        u8 pad[3];
        AccountUid uid;
        u8 unk2[0x400 - 0x18];
    };

    static_assert(sizeof(ApplicationSelectedUserArgument) == 0x400, "ApplicationSelectedUserArgument must be 0x400!");

    static constexpr u32 SelectedUserMagic = 0xC79497CA;
    static constexpr u64 QHbTargetSystemApplicationId = OS_FLOG_APP_ID;

    bool ApplicationIsActive();
    void ApplicationTerminate();
    Result ApplicationStart(u64 app_id, bool system, AccountUid user_id, void *data = NULL, size_t size = 0);
    bool ApplicationHasForeground();
    Result ApplicationSetForeground();
    Result ApplicationSend(void *data, size_t size, AppletLaunchParameterKind kind = AppletLaunchParameterKind_UserChannel);
    u64 ApplicationGetId();

    bool ApplicationNeedsUser(u64 app_id);
}
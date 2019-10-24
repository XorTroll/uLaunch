
#pragma once
#include <q_Include.hpp>
#include <os/os_Titles.hpp>

namespace am
{
    struct ApplicationSelectedUserArgument
    {
        u32 magic;
        u8 one;
        u8 pad[3];
        u128 uid;
        u8 unk2[0x70];
    } PACKED;

    static constexpr u32 SelectedUserMagic = 0xC79497CA;
    static constexpr u64 QHbTargetSystemApplicationId = OS_FLOG_APP_ID;

    bool ApplicationIsActive();
    void ApplicationTerminate();
    Result ApplicationStart(u64 app_id, bool system, u128 user_id, void *data = NULL, size_t size = 0);
    bool ApplicationHasForeground();
    Result ApplicationSetForeground();
    Result ApplicationSend(void *data, size_t size, AppletLaunchParameterKind kind = AppletLaunchParameterKind_UserChannel);
    u64 ApplicationGetId();
}
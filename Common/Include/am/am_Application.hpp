
#pragma once
#include <q_Include.hpp>

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

    bool ApplicationIsActive();
    void ApplicationTerminate();
    Result ApplicationStart(u64 app_id, bool system, u128 user_id);
    bool ApplicationHasForeground();
    Result ApplicationSetForeground();
}
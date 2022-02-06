
#pragma once
#include <ul_Include.hpp>
#include <os/os_Titles.hpp>

namespace am {

    struct ApplicationSelectedUserArgument {
        static constexpr u32 Magic = 0xC79497CA;

        u32 magic;
        u8 unk_1;
        u8 pad[3];
        AccountUid uid;
        u8 unk_2[0x3E8];

        static inline constexpr ApplicationSelectedUserArgument Create(const AccountUid uid) {
            return {
                .magic = Magic,
                .unk_1 = 1,
                .uid = uid
            };
        }
    };
    static_assert(sizeof(ApplicationSelectedUserArgument) == 0x400);

    bool ApplicationIsActive();
    void ApplicationTerminate();
    Result ApplicationStart(const u64 app_id, const bool system, const AccountUid user_id, const void *data = nullptr, const size_t size = 0);
    bool ApplicationHasForeground();
    Result ApplicationSetForeground();
    Result ApplicationSend(const void *data, const size_t size, const AppletLaunchParameterKind kind = AppletLaunchParameterKind_UserChannel);
    u64 ApplicationGetId();

    bool ApplicationNeedsUser(const u64 app_id);

}
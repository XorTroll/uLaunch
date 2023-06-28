
#pragma once
#include <switch.h>

namespace ul::system::app {

    struct ApplicationSelectedUserArgument {
        static constexpr u32 Magic = 0xC79497CA;

        u32 magic;
        u8 unk_1; // Maybe bool is_user_selected?
        u8 pad[3];
        AccountUid uid;
        u8 unused[0x70];

        static inline constexpr ApplicationSelectedUserArgument Create(const AccountUid uid) {
            return {
                .magic = Magic,
                .unk_1 = 1,
                .uid = uid
            };
        }
    };
    static_assert(sizeof(ApplicationSelectedUserArgument) == 0x88);

    bool IsActive();
    Result Terminate();
    Result Start(const u64 app_id, const bool system, const AccountUid user_id, const void *data = nullptr, const size_t size = 0);
    bool HasForeground();
    Result SetForeground();
    Result Send(const void *data, const size_t size, const AppletLaunchParameterKind kind = AppletLaunchParameterKind_UserChannel);
    u64 GetId();

}
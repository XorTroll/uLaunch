
#pragma once
#include <switch.h>

namespace ul::loader {

    enum class EntryKind : u32 {
        EndOfList = 0,
        MainThreadHandle = 1,
        NextLoadPath = 2,
        OverrideHeap = 3,
        OverrideService = 4,
        Argv = 5,
        SyscallAvailableHint = 6,
        AppletType = 7,
        AppletWorkaround = 8,
        Reserved = 9,
        ProcessHandle = 10,
        LastLoadResult = 11,
        RandomSeed = 14,
        UserIdStorage = 15,
        HosVersion = 16
    };

    enum class EntryFlags : u32 {
        None = 0,
        Mandatory = BIT(0)
    };

    enum class EntryAppletFlags : u32 {
        None = 0,
        ApplicationOverride = BIT(0)
    };

    struct ConfigEntry {
        EntryKind kind;
        u32 flags;
        u64 value[2];
    };

}
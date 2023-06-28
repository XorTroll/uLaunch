
#pragma once
#include <switch.h>

namespace ul::system::la {

    bool IsActive();
    Result Terminate();
    Result Start(const AppletId id, const u32 la_version, const void *in_data, const size_t in_size);
    Result Send(const void *data, const size_t size);
    Result Read(void *data, const size_t size);
    Result Push(AppletStorage *st);
    Result Pop(AppletStorage *st);
    
    inline Result StartWeb(WebCommonConfig *web) {
        return Start(AppletId_LibraryAppletWeb, web->version, &web->arg, sizeof(web->arg));
    }

    u64 GetProgramIdForAppletId(const AppletId id);
    AppletId GetAppletIdForProgramId(const u64 id);

    AppletId GetLastAppletId();

    bool IsMenu();
    void SetMenuAppletId(const AppletId id);
    AppletId GetMenuAppletId();

    inline void SetMenuProgramId(const u64 id) {
        SetMenuAppletId(GetAppletIdForProgramId(id));
    }

    inline u64 GetMenuProgramId() {
        return GetProgramIdForAppletId(GetMenuAppletId());
    }

}

#pragma once
#include <ul_Include.hpp>

namespace am {

    bool LibraryAppletIsActive();
    void LibraryAppletTerminate();
    Result LibraryAppletStart(const AppletId id, const u32 la_version, const void *in_data, const size_t in_size);
    Result LibraryAppletSend(const void *data, const size_t size);
    Result LibraryAppletRead(void *data, const size_t size);
    Result LibraryAppletPush(AppletStorage *st);
    Result LibraryAppletPop(AppletStorage *st);
    
    inline Result WebAppletStart(WebCommonConfig *web) {
        return LibraryAppletStart(AppletId_LibraryAppletWeb, web->version, &web->arg, sizeof(web->arg));
    }

    u64 LibraryAppletGetProgramIdForAppletId(const AppletId id);
    AppletId LibraryAppletGetAppletIdForProgramId(const u64 id);

    AppletId LibraryAppletGetId();

    bool LibraryAppletIsMenu();
    void LibraryAppletSetMenuAppletId(const AppletId id);
    AppletId LibraryAppletGetMenuAppletId();

    inline void LibraryAppletSetMenuProgramId(const u64 id) {
        LibraryAppletSetMenuAppletId(LibraryAppletGetAppletIdForProgramId(id));
    }

    inline u64 LibraryAppletGetMenuProgramId() {
        return LibraryAppletGetProgramIdForAppletId(LibraryAppletGetMenuAppletId());
    }

}
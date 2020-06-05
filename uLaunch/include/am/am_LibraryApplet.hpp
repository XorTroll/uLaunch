
#pragma once
#include <ul_Include.hpp>

namespace am {

    bool LibraryAppletIsActive();
    void LibraryAppletSetMenuAppletId(AppletId id);
    AppletId LibraryAppletGetMenuAppletId();
    bool LibraryAppletIsMenu();
    void LibraryAppletTerminate();
    Result LibraryAppletStart(AppletId id, u32 la_version, void *in_data, size_t in_size);
    Result LibraryAppletSend(void *data, size_t size);
    Result LibraryAppletRead(void *data, size_t size);
    
    inline Result WebAppletStart(WebCommonConfig *web) {
        return LibraryAppletStart(AppletId_web, web->version, &web->arg, sizeof(web->arg));
    }

    u64 LibraryAppletGetProgramIdForAppletId(AppletId id);
    AppletId LibraryAppletGetAppletIdForProgramId(u64 id);

    AppletId LibraryAppletGetId();

    static constexpr AppletId InvalidAppletId = static_cast<AppletId>(0);

}
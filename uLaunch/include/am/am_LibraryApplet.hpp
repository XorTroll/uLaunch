
#pragma once
#include <ul_Include.hpp>
#include <map>

namespace am
{
    bool LibraryAppletIsActive();
    void LibraryAppletSetMenuAppletId(AppletId id);
    bool LibraryAppletIsMenu();
    void LibraryAppletTerminate();
    Result LibraryAppletStart(AppletId id, u32 la_version, void *in_data, size_t in_size);
    Result LibraryAppletSend(void *data, size_t size);
    Result LibraryAppletRead(void *data, size_t size);
    Result WebAppletStart(WebCommonConfig *web);

    u64 LibraryAppletGetProgramIdForAppletId(AppletId id);
    AppletId LibraryAppletGetAppletIdForProgramId(u64 id);

    AppletId LibraryAppletGetId();

    static constexpr AppletId InvalidAppletId = (AppletId)0;
}
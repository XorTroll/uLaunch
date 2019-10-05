
#pragma once
#include <q_Include.hpp>

namespace am
{
    bool LibraryAppletIsActive();
    bool LibraryAppletIsQMenu();
    void LibraryAppletTerminate();
    Result LibraryAppletStart(AppletId id, u32 la_version, void *in_data, size_t in_size);
    Result LibraryAppletSend(void *data, size_t size);
    Result LibraryAppletRead(void *data, size_t size);
}
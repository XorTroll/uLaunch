#include <am/am_HomeMenu.hpp>
#include <am/am_LibraryApplet.hpp>

namespace am
{
    bool home_focus = true;
    extern AppletHolder applet_holder;

    bool HomeMenuHasForeground()
    {
        return home_focus;
    }

    static Result _appletAccessorRequestForAppletToGetForeground(Service *srv)
    {
        IpcCommand c;
        ipcInitialize(&c);

        struct {
            u64 magic;
            u64 cmdid;
        } *raw = (decltype(raw))serviceIpcPrepareHeader(srv, &c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmdid = 150;

        auto rc = serviceIpcDispatch(srv);

        if(R_SUCCEEDED(rc))
        {
            IpcParsedCommand r;
            struct {
                u64 magic;
                u64 res;
            } *resp;

            serviceIpcParse(srv, &r, sizeof(*resp));

            resp = (decltype(resp))r.Raw;

            rc = resp->res;
        }

        return rc;
    }

    Result HomeMenuSetForeground()
    {
        Result rc = 0xDEAD;
        if(LibraryAppletIsQMenu())
        {
            rc = _appletAccessorRequestForAppletToGetForeground(&applet_holder.s);
            if(R_SUCCEEDED(rc)) home_focus = true;
        }
        else
        {
            rc = appletRequestToGetForeground();
            if(R_SUCCEEDED(rc)) home_focus = true;
        }
        return rc;
    }
}
#include <net/net_Service.hpp>

namespace net
{
    Service nifmsrv;
    Service nifmgeneralsrv;
    bool init = false;

    Result Initialize()
    {
        if(serviceIsActive(&nifmsrv)) return 0;
        auto rc = smGetService(&nifmsrv, "nifm:u");
        if(R_SUCCEEDED(rc))
        {
            if(serviceIsActive(&nifmgeneralsrv)) serviceClose(&nifmgeneralsrv);
            IpcCommand c;
            ipcInitialize(&c);
            struct
            {
                u64 magic;
                u64 cmdid;
            } *in = (decltype(in))ipcPrepareHeader(&c, sizeof(*in));
            in->magic = SFCI_MAGIC;
            in->cmdid = 4;

            auto rc = serviceIpcDispatch(&nifmsrv);
            if(R_SUCCEEDED(rc))
            {
                IpcParsedCommand r;
                ipcParse(&r);

                struct
                {
                    u64 magic;
                    u64 res;
                } *res = (decltype(res))r.Raw;

                rc = res->res;
                if(R_SUCCEEDED(rc))
                {
                    serviceCreate(&nifmgeneralsrv, r.Handles[0]);
                }
            }
        }
        return rc;
    }

    void Finalize()
    {
        if(serviceIsActive(&nifmgeneralsrv)) serviceClose(&nifmgeneralsrv);
        if(serviceIsActive(&nifmsrv)) serviceClose(&nifmsrv);
    }

    bool HasConnection()
    {
        IpcCommand c;
        ipcInitialize(&c);
        struct
        {
            u64 magic;
            u64 cmdid;
        } *in = (decltype(in))ipcPrepareHeader(&c, sizeof(*in));
        in->magic = SFCI_MAGIC;
        in->cmdid = 18;

        auto rc = serviceIpcDispatch(&nifmgeneralsrv);
        if(R_SUCCEEDED(rc))
        {
            IpcParsedCommand r;
            ipcParse(&r);

            struct
            {
                u64 magic;
                u64 res;
                u8 type;
                u8 strength;
                u8 status;
            } *res = (decltype(res))r.Raw;

            rc = res->res;
            if(R_SUCCEEDED(rc)) return ((NifmInternetConnectionStatus)res->status == NifmInternetConnectionStatus_Connected);
        }
        return false;
    }

    Result GetCurrentNetworkProfile(NetworkProfileData *data)
    {
        IpcCommand c;
        ipcInitialize(&c);
        ipcAddRecvStatic(&c, data, sizeof(NetworkProfileData), 0);

        struct
        {
            u64 magic;
            u64 cmdid;
        } *in = (decltype(in))ipcPrepareHeader(&c, sizeof(*in));
        in->magic = SFCI_MAGIC;
        in->cmdid = 5;

        auto rc = serviceIpcDispatch(&nifmgeneralsrv);
        if(R_SUCCEEDED(rc))
        {
            IpcParsedCommand r;
            ipcParse(&r);

            struct
            {
                u64 magic;
                u64 res;
            } *res = (decltype(res))r.Raw;

            rc = res->res;
        }
        return rc;
    }
}
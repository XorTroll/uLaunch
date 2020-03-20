#include <net/net_Service.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace net
{
    Result Initialize()
    {
        auto rc = nifmInitialize(NifmServiceType_System);
        if(R_SUCCEEDED(rc)) rc = wlaninfInitialize();
        return rc;
    }

    void Finalize()
    {
        wlaninfExit();
        nifmExit();
    }

    bool HasConnection()
    {
        NifmInternetConnectionStatus status = NifmInternetConnectionStatus_ConnectingUnknown1;
        nifmGetInternetConnectionStatus(NULL, NULL, &status);
        return (status == NifmInternetConnectionStatus_Connected);
    }

    Result GetCurrentNetworkProfile(NetworkProfileData *data)
    {
        return serviceDispatch(nifmGetServiceSession_GeneralService(), 5,
            .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_Out | SfBufferAttr_HipcPointer },
            .buffers = { { data, sizeof(NetworkProfileData) } }
        );
    }

    Result GetMACAddress(u64 *out)
    {
        return serviceDispatchOut(wlaninfGetServiceSession(), 2, *out);
    }

    std::string FormatMACAddress(u64 addr)
    {
        std::stringstream strm;
        strm << std::hex << std::uppercase << addr;
        std::string str;
        auto sstrm = strm.str();
        for(u32 i = 1; i < 7; i++)
        {
            str += sstrm.substr((6 - i) * 2, 2);
            if(i < 6) str += ":";
        }
        return str;
    }

    std::string GetConsoleIPAddress()
    {
        char ipaddr[0x20] = {0};
        auto ip = gethostid();
        inet_ntop(AF_INET, &ip, ipaddr, sizeof(ipaddr));
        return ipaddr;
    }
}
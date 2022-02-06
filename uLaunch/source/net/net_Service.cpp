#include <net/net_Service.hpp>
#include <unistd.h>

namespace net {

    Result Initialize() {
        UL_RC_TRY(nifmInitialize(NifmServiceType_System));
        UL_RC_TRY(wlaninfInitialize());
        return ResultSuccess;
    }

    void Finalize() {
        wlaninfExit();
        nifmExit();
    }

    bool HasConnection() {
        auto status = NifmInternetConnectionStatus_ConnectingUnknown1;
        nifmGetInternetConnectionStatus(nullptr, nullptr, &status);
        return status == NifmInternetConnectionStatus_Connected;
    }

    Result GetCurrentNetworkProfile(NetworkProfileData *data) {
        return serviceDispatch(nifmGetServiceSession_GeneralService(), 5,
            .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_Out | SfBufferAttr_HipcPointer },
            .buffers = { { data, sizeof(NetworkProfileData) } }
        );
    }

    Result GetMACAddress(u64 *out) {
        return serviceDispatchOut(wlaninfGetServiceSession(), 2, *out);
    }

    std::string FormatMACAddress(u64 addr) {
        std::stringstream strm;
        strm << std::hex << std::uppercase << addr;
        std::string str;
        auto sstrm = strm.str();
        for(u32 i = 1; i < 7; i++) {
            str += sstrm.substr((6 - i) * 2, 2);
            if(i < 6) {
                str += ":";
            }
        }
        return str;
    }

    std::string GetConsoleIPAddress() {
        char ipaddr[0x20] = {0};
        auto ip = gethostid();
        sprintf(ipaddr, "%lu.%lu.%lu.%lu", (ip & 0x000000FF), (ip & 0x0000FF00) >> 8, (ip & 0x00FF0000) >> 16, (ip & 0xFF000000) >> 24);
        return ipaddr;
    }

}
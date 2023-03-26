#include <net/net_Service.hpp>
#include <unistd.h>

namespace net {

    namespace {

        Service g_WlanService;
        Service g_WlanWirelessCommunicationService;

        Result wlanInitialize() {
            if(serviceIsActive(&g_WlanService)) {
                return ResultSuccess;
            }

            UL_RC_TRY(smGetService(&g_WlanService, "wlan"));

            // https://switchbrew.org/wiki/WLAN_services#CreateWirelessCommunicationService
            UL_RC_TRY(serviceDispatch(&g_WlanService, 0, 
                .out_num_objects = 1,
                .out_objects = &g_WlanWirelessCommunicationService
            ));

            return ResultSuccess;
        }

        void wlanExit() {
            serviceClose(&g_WlanWirelessCommunicationService);
            serviceClose(&g_WlanService);
        }

        Result wlaninfGetMacAddress(WlanMacAddress *out_addr) {
            return serviceDispatchOut(wlaninfGetServiceSession(), 2, *out_addr);
        }

        Result wlanGetMacAddress(WlanMacAddress *out_addr) {
            return serviceDispatchOut(&g_WlanWirelessCommunicationService, 33, *out_addr);
        }

    }

    Result Initialize() {
        UL_RC_TRY(nifmInitialize(NifmServiceType_System));

        if(hosversionAtLeast(15,0,0)) {
            UL_RC_TRY(wlanInitialize());
        }
        else {
            UL_RC_TRY(wlaninfInitialize());
        }

        return ResultSuccess;
    }

    void Finalize() {
        if(hosversionAtLeast(15,0,0)) {
            wlanExit();
        }
        else {
            wlaninfExit();
        }
        
        nifmExit();
    }

    bool HasConnection() {
        auto status = NifmInternetConnectionStatus_ConnectingUnknown1;
        nifmGetInternetConnectionStatus(nullptr, nullptr, &status);
        return status == NifmInternetConnectionStatus_Connected;
    }

    Result GetCurrentNetworkProfile(NetworkProfileData &out_prof_data) {
        return serviceDispatch(nifmGetServiceSession_GeneralService(), 5,
            .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_Out | SfBufferAttr_HipcPointer },
            .buffers = { { std::addressof(out_prof_data), sizeof(out_prof_data) } }
        );
    }

    Result GetMacAddress(WlanMacAddress &out_addr) {
        if(hosversionAtLeast(15,0,0)) {
            UL_RC_TRY(wlanGetMacAddress(std::addressof(out_addr)));
        }
        else {
            UL_RC_TRY(wlaninfGetMacAddress(std::addressof(out_addr)));
        }
        return 0;
    }

    std::string FormatMacAddress(const WlanMacAddress &addr) {
        std::stringstream strm;
        strm << std::hex << std::uppercase;
        for(u32 i = 0; i < sizeof(WlanMacAddress); i++) {
            strm << static_cast<u32>(addr.mac[i]);
            if((i + 1) < sizeof(WlanMacAddress)) {
                strm << ':';
            }
        }
        return strm.str();
    }

    std::string GetConsoleIpAddress() {
        char ip_addr[0x20] = {0};
        auto ip = gethostid();
        sprintf(ip_addr, "%lu.%lu.%lu.%lu", ip & 0x000000FF, (ip & 0x0000FF00) >> 8, (ip & 0x00FF0000) >> 16, (ip & 0xFF000000) >> 24);
        return ip_addr;
    }

}
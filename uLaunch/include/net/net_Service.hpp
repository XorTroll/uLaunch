
#pragma once
#include <net/net_Types.hpp>

namespace net {

    struct __attribute__((aligned(1))) WlanMacAddress {
        u8 mac[0x6];
    };

    Result Initialize();
    void Finalize();
    Result GetInternetConnectionStatus(NifmInternetConnectionStatus &out_status);
    bool HasConnection();
    Result GetCurrentNetworkProfile(NetworkProfileData &out_prof_data);
    Result GetMacAddress(WlanMacAddress &out_addr);

    std::string FormatMacAddress(const WlanMacAddress &addr);
    std::string GetConsoleIpAddress();

}
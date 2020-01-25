
#pragma once
#include <net/net_Types.hpp>

namespace net
{
    Result Initialize();
    void Finalize();
    Result GetInternetConnectionStatus(NifmInternetConnectionStatus* status);
    bool HasConnection();
    Result GetCurrentNetworkProfile(NetworkProfileData *data);
    Result GetMACAddress(u64 *out);

    std::string FormatMACAddress(u64 addr);
}
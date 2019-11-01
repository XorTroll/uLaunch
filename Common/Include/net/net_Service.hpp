
#pragma once
#include <net/net_Types.hpp>

namespace net
{
    Result Initialize();
    void Finalize();
    Result GetInternetConnectionStatus(NifmInternetConnectionStatus* status);
    bool HasConnection();
    Result GetCurrentNetworkProfile(NetworkProfileData *data);
}
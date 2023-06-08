
#pragma once
#include <ul/ul_Include.hpp>

namespace ul::net {

    struct __attribute__((aligned(1))) WlanMacAddress {
        u8 mac[0x6];
    };

    Result Initialize();
    void Finalize();
    Result GetInternetConnectionStatus(NifmInternetConnectionStatus &out_status);
    bool HasConnection();
    Result GetMacAddress(WlanMacAddress &out_addr);

    std::string FormatMacAddress(const WlanMacAddress &addr);
    std::string GetConsoleIpAddress();

}

#pragma once
#include <ul_Include.hpp>

namespace net {

    // TODO: use libnx's nifm stuff?
    struct NetworkProfileData {
        u8 unk[0x117];
        char wifi_name[0x20 + 1];
        u8 unk2[3];
        char wifi_password[0x40 + 1];
    };
    static_assert(sizeof(NetworkProfileData) == 0x17C, "Invalid NetworkProfileData");

}
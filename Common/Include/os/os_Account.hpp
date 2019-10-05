
#pragma once
#include <q_Include.hpp>

namespace os
{
    std::string GetIconCacheImagePath(u128 user_id);
    ResultWith<std::vector<u128>> QuerySystemAccounts(bool dump_icon);
    ResultWith<std::string> GetAccountName(u128 user_id);
}
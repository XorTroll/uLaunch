
#pragma once
#include <q_Include.hpp>

namespace os
{
    std::string GetIconCacheImagePath(AccountUid user_id);
    ResultWith<std::vector<AccountUid>> QuerySystemAccounts(bool dump_icon);
    ResultWith<std::string> GetAccountName(AccountUid user_id);
}
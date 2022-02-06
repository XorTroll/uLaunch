
#pragma once
#include <ul_Include.hpp>

namespace os {

    std::string GetIconCacheImagePath(const AccountUid user_id);
    Result QuerySystemAccounts(const bool dump_icon, std::vector<AccountUid> &out_accounts);
    Result GetAccountName(const AccountUid user_id, std::string &out_name);

}

#pragma once
#include <ul_Include.hpp>

namespace os {

    std::string GetIconCacheImagePath(AccountUid user_id);
    Result QuerySystemAccounts(std::vector<AccountUid> &out_accounts, bool dump_icon);
    Result GetAccountName(std::string &out_name, AccountUid user_id);

}

#pragma once
#include <ul/ul_Include.hpp>

namespace ul::acc {

    Result ListAccounts(std::vector<AccountUid> &out_accounts);
    Result GetAccountName(const AccountUid user_id, std::string &out_name);

    std::string GetIconCacheImagePath(const AccountUid user_id);
    Result CacheAccounts();

}

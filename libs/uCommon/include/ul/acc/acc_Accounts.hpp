
#pragma once
#include <ul/ul_Include.hpp>

namespace ul::acc {

    Result ListAccounts(std::vector<AccountUid> &out_accounts);
    Result GetAccountName(const AccountUid user_id, std::string &out_name);
    Result LoadAccountImage(const AccountUid user_id, u8 *&out_img_data, size_t &out_img_size);

}

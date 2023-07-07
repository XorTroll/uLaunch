#include <ul/acc/acc_Accounts.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_String.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Scope.hpp>

namespace ul::acc {

    std::string GetIconCacheImagePath(const AccountUid user_id) {
        const auto uid_str = util::FormatAccount(user_id);
        return JoinPath(AccountCachePath, uid_str + ".jpg");
    }

    Result ListAccounts(std::vector<AccountUid> &out_accounts) {
        AccountUid uids[ACC_USER_LIST_SIZE] = {};
        s32 acc_count = 0;
        UL_RC_TRY(accountListAllUsers(uids, ACC_USER_LIST_SIZE, &acc_count));
        for(s32 i = 0; i < acc_count; i++) {
            out_accounts.push_back(uids[i]);
        }
        return ResultSuccess;
    }

    Result CacheAccounts() {
        fs::CleanDirectory(AccountCachePath);

        AccountUid uids[ACC_USER_LIST_SIZE] = {};
        s32 acc_count = 0;
        UL_RC_TRY(accountListAllUsers(uids, ACC_USER_LIST_SIZE, &acc_count));
        for(s32 i = 0; i < acc_count; i++) {
            const auto uid = uids[i];
            AccountProfile prof;
            if(R_SUCCEEDED(accountGetProfile(&prof, uid))) {
                u32 img_size = 0;
                accountProfileGetImageSize(&prof, &img_size);
                if(img_size > 0) {
                    auto img_buf = new u8[img_size]();
                    u32 tmp_size;
                    if(R_SUCCEEDED(accountProfileLoadImage(&prof, img_buf, img_size, &tmp_size))) {
                        const auto cache_icon_path = GetIconCacheImagePath(uid);
                        fs::WriteFile(cache_icon_path, img_buf, img_size, true);
                    }
                    delete[] img_buf;
                }
                accountProfileClose(&prof);
            }
        }
        return ResultSuccess;
    }

    Result GetAccountName(const AccountUid user_id, std::string &out_name) {
        AccountProfile prof;
        UL_RC_TRY(accountGetProfile(&prof, user_id));
        UL_ON_SCOPE_EXIT({ accountProfileClose(&prof); });
    
        AccountProfileBase pbase;
        AccountUserData udata;
        UL_RC_TRY(accountProfileGet(&prof, &udata, &pbase));

        out_name = pbase.nickname;
        return ResultSuccess;
    }
}
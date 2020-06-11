#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>

namespace os {

    std::string GetIconCacheImagePath(AccountUid user_id) {
        auto uidstr = util::Format128NintendoStyle(user_id);
        return UL_BASE_SD_DIR "/user/" + uidstr + ".jpg";
    }

    Result QuerySystemAccounts(std::vector<AccountUid> &out_accounts, bool dump_icon) {
        AccountUid uids[ACC_USER_LIST_SIZE] = {};
        s32 acc_count = 0;
        R_TRY(accountListAllUsers(uids, ACC_USER_LIST_SIZE, &acc_count));
        for(s32 i = 0; i < acc_count; i++) {
            out_accounts.push_back(uids[i]);
            if(dump_icon) {
                AccountProfile prof;
                auto rc = accountGetProfile(&prof, uids[i]);
                if(R_SUCCEEDED(rc)) {
                    u32 imgsz = 0;
                    rc = accountProfileGetImageSize(&prof, &imgsz);
                    if(imgsz > 0) {
                        auto imgbuf = new u8[imgsz]();
                        u32 tmpsz;
                        rc = accountProfileLoadImage(&prof, imgbuf, imgsz, &tmpsz);
                        if(R_SUCCEEDED(rc)) {
                            auto iconcache = GetIconCacheImagePath(uids[i]);
                            fs::WriteFile(iconcache, imgbuf, imgsz, true);
                        }
                        delete[] imgbuf;
                    }
                    accountProfileClose(&prof);
                }
            }
        }
        return ResultSuccess;
    }

    Result GetAccountName(std::string &out_name, AccountUid user_id) {
        std::string name;
        AccountProfile prof;
        R_TRY(accountGetProfile(&prof, user_id));
        UL_ON_SCOPE_EXIT({
            accountProfileClose(&prof);
        });
        AccountProfileBase pbase;
        AccountUserData udata;
        R_TRY(accountProfileGet(&prof, &udata, &pbase));
        out_name = pbase.nickname;
        return ResultSuccess;
    }
}
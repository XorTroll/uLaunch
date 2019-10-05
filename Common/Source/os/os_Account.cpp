#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>

namespace os
{
    std::string GetIconCacheImagePath(u128 user_id)
    {
        auto uidstr = util::Format128NintendoStyle(user_id);
        return Q_BASE_SD_DIR "/user/" + uidstr + ".jpg";
    }

    ResultWith<std::vector<u128>> QuerySystemAccounts(bool dump_icon)
    {
        std::vector<u128> uids;
        u128 *uidbuf = new u128[ACC_USER_LIST_SIZE]();
        size_t acc_count = 0;
        auto rc = accountListAllUsers(uidbuf, ACC_USER_LIST_SIZE, &acc_count);
        if(R_SUCCEEDED(rc))
        {
            for(u32 i = 0; i < acc_count; i++)
            {
                uids.push_back(uidbuf[i]);
                if(dump_icon)
                {
                    AccountProfile prof;
                    rc = accountGetProfile(&prof, uidbuf[i]);
                    if(R_SUCCEEDED(rc))
                    {
                        size_t imgsz = 0;
                        rc = accountProfileGetImageSize(&prof, &imgsz);
                        if(imgsz > 0)
                        {
                            u8 *imgbuf = new u8[imgsz]();
                            size_t tmpsz;
                            rc = accountProfileLoadImage(&prof, imgbuf, imgsz, &tmpsz);
                            if(R_SUCCEEDED(rc))
                            {
                                auto iconcache = GetIconCacheImagePath(uidbuf[i]);
                                fs::DeleteFile(iconcache);
                                FILE *f = fopen(iconcache.c_str(), "wb");
                                if(f)
                                {
                                    fwrite(imgbuf, 1, imgsz, f);
                                    fclose(f);
                                }
                            }
                        }
                    }
                }
            }
        }
        delete[] uidbuf;
        return MakeResultWith(rc, uids);
    }

    ResultWith<std::string> GetAccountName(u128 user_id)
    {
        std::string name;
        AccountProfile prof;
        R_TRY_WITH(accountGetProfile(&prof, user_id), name);
        AccountProfileBase pbase;
        AccountUserData udata;
        R_TRY_WITH(accountProfileGet(&prof, &udata, &pbase), name);
        name = pbase.username;
        accountProfileClose(&prof);
        return SuccessResultWith(name);
    }
}
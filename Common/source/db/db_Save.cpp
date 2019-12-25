#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>
#include <util/util_Convert.hpp>

namespace db
{
    Result Mount()
    {
        FsFileSystem savefs;
        FsSaveDataAttribute attr = {};
        attr.system_save_data_id = HomeMenuSaveDataId;
        attr.save_data_type = FsSaveDataType_System;

        auto rc = fsOpenSaveDataFileSystemBySystemSaveDataId(&savefs, FsSaveDataSpaceId_System, &attr);
        if(R_SUCCEEDED(rc)) fsdevMountDevice(Q_DB_MOUNT_NAME, savefs);
        
        return rc;
    }

    void Unmount()
    {
        fsdevUnmountDevice(Q_DB_MOUNT_NAME);
    }

    void Commit()
    {
        fsdevCommitDevice(Q_DB_MOUNT_NAME);
    }

    ResultWith<PassBlock> PackPassword(AccountUid uid, std::string pass)
    {
        PassBlock pb = {};
        if((pass.length() > 15) || (pass.empty())) return MakeResultWith(RES_VALUE(Db, InvalidPasswordLength), pb);
        memcpy(&pb.uid, &uid, sizeof(AccountUid));
        char passbuf[0x10] = {0};
        strcpy(passbuf, pass.c_str());
        sha256CalculateHash(pb.pass_sha, passbuf, 0x10);
        return SuccessResultWith(pb);
    }

    ResultWith<PassBlock> AccessPassword(AccountUid user_id)
    {
        PassBlock pb = {};
        auto filename = GetUserPasswordFilePath(user_id);
        if(fs::ExistsFile(filename))
        {
            if(fs::ReadFile(filename, &pb, sizeof(pb))) return SuccessResultWith(pb);
        }
        return MakeResultWith(RES_VALUE(Db, PasswordNotFound), pb);
    }

    std::string GetUserPasswordFilePath(AccountUid user_id)
    {
        auto uidstr = util::Format128NintendoStyle(user_id);
        return Q_BASE_DB_DIR "/user/" + uidstr + ".pass";
    }

    Result RegisterUserPassword(PassBlock password)
    {
        std::string pwd;
        auto filename = GetUserPasswordFilePath(password.uid);
        if(fs::ExistsFile(filename)) return RES_VALUE(Db, PasswordAlreadyExists);
        if(fs::WriteFile(filename, &password, sizeof(password), true))
        {
            db::Commit();
            return 0;
        }
        return RES_VALUE(Db, PasswordWriteFail);
    }

    Result RemoveUserPassword(AccountUid uid)
    {
        auto passfile = GetUserPasswordFilePath(uid);
        if(!fs::ExistsFile(passfile)) return RES_VALUE(Db, PasswordNotFound);
        fs::DeleteFile(passfile);
        db::Commit();
        return 0;
    }

    Result TryLogUser(PassBlock password)
    {
        auto [rc, pwd] = AccessPassword(password.uid);
        if(R_SUCCEEDED(rc))
        {
            if(memcmp(&password.uid, &pwd.uid, sizeof(AccountUid)) != 0) return RES_VALUE(Db, PasswordUserMismatch);
            if(memcmp(password.pass_sha, pwd.pass_sha, 0x20) != 0) return RES_VALUE(Db, PasswordMismatch);
        }
        return rc;
    }
}
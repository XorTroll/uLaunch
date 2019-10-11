#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>
#include <util/util_Convert.hpp>

namespace db
{
    Result Mount()
    {
        FsFileSystem savefs;
        Result rc = 0;
        do
        {
            // Ensure we success mounting it (we should be able to do so)
            rc = fsMount_SystemSaveData(&savefs, HomeMenuSaveDataId);
        } while(R_FAILED(rc));
        fsdevMountDevice(Q_DB_MOUNT_NAME, savefs);
        return 0;
    }

    void Unmount()
    {
        fsdevUnmountDevice(Q_DB_MOUNT_NAME);
    }

    void Commit()
    {
        fsdevCommitDevice(Q_DB_MOUNT_NAME);
    }

    ResultWith<PassBlock> AccessPassword(u128 user_id)
    {
        PassBlock pb = {};
        auto filename = GetUserPasswordFilePath(user_id);
        if(fs::ExistsFile(filename))
        {
            if(fs::ReadFile(filename, &pb, sizeof(pb))) return SuccessResultWith(pb);
        }
        return MakeResultWith(0xdead, pb);
    }

    std::string GetUserPasswordFilePath(u128 user_id)
    {
        auto uidstr = util::Format128NintendoStyle(user_id);
        return Q_BASE_DB_DIR "/user/" + uidstr + ".pass";
    }

    Result RegisterUserPassword(u128 user_id, std::string password)
    {
        std::string pwd;
        auto filename = GetUserPasswordFilePath(user_id);
        if(fs::ExistsFile(filename)) return 0xdead;

        if((password.length() > 15) || (password.empty())) return 0xdead1;
        PassBlock pb = {};
        memcpy(&pb.uid, &user_id, sizeof(u128));
        char tmppass[0x10] = {0};
        strcpy(tmppass, password.c_str());
        sha256CalculateHash(pb.pass_sha, tmppass, 0x10);

        if(!fs::WriteFile(filename, &pb, sizeof(pb), true)) return 0xdead2;
        return 0;
    }

    Result TryLogUser(u128 user_id, std::string password)
    {
        if((password.length() > 15) || (password.empty())) return 0xdead1;
        auto [rc, pwd] = AccessPassword(user_id);
        if(R_SUCCEEDED(rc))
        {
            u8 tmpsha[0x20] = {0};
            char tmppass[0x10] = {0};
            strcpy(tmppass, password.c_str());
            sha256CalculateHash(tmpsha, tmppass, 0x10);
            if(memcmp(&user_id, &pwd.uid, sizeof(u128)) != 0) return 0xdead3;
            if(memcmp(tmpsha, pwd.pass_sha, 0x20) != 0) return 0xdead4;
        }
        else return 0xdead2;
        return 0;
    }
}
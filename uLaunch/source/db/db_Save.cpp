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
        if(R_SUCCEEDED(rc)) fsdevMountDevice(UL_DB_MOUNT_NAME, savefs);
        
        return rc;
    }

    void Unmount()
    {
        fsdevUnmountDevice(UL_DB_MOUNT_NAME);
    }

    void Commit()
    {
        fsdevCommitDevice(UL_DB_MOUNT_NAME);
    }
}
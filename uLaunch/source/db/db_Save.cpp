#include <db/db_Save.hpp>

namespace db {

    Result Mount() {
        FsFileSystem savefs;
        FsSaveDataAttribute attr = {};
        attr.system_save_data_id = HomeMenuSaveDataId;
        attr.save_data_type = FsSaveDataType_System;

        UL_RC_TRY(fsOpenSaveDataFileSystemBySystemSaveDataId(&savefs, FsSaveDataSpaceId_System, &attr));
        fsdevMountDevice(UL_DB_MOUNT_NAME, savefs);
        
        return ResultSuccess;
    }

    void Unmount() {
        fsdevUnmountDevice(UL_DB_MOUNT_NAME);
    }

    void Commit() {
        fsdevCommitDevice(UL_DB_MOUNT_NAME);
    }

}
#include <am/am_LibraryAppletUtils.hpp>

namespace am {

    Result ReadDataFromStorage(void *data, size_t data_size) {
        AppletStorage st;
        R_TRY(appletPopInData(&st));
        UL_ON_SCOPE_EXIT({
            appletStorageClose(&st);
        });
        R_TRY(appletStorageRead(&st, 0, data, data_size));
        return ResultSuccess;
    }

    Result ReadStartMode(dmi::MenuStartMode &out_start_mode) {
        LibAppletArgs args = {};
        R_TRY(ReadDataFromStorage(&args, sizeof(args)));
        out_start_mode = static_cast<dmi::MenuStartMode>(args.LaVersion);
        return ResultSuccess;
    }

}
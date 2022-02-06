#include <am/am_LibraryAppletUtils.hpp>

namespace am {

    Result ReadDataFromStorage(void *out_data, const size_t data_size) {
        AppletStorage st;
        UL_RC_TRY(appletPopInData(&st));
        UL_ON_SCOPE_EXIT({ appletStorageClose(&st); });

        UL_RC_TRY(appletStorageRead(&st, 0, out_data, data_size));
        return ResultSuccess;
    }

    Result ReadStartMode(dmi::MenuStartMode &out_start_mode) {
        LibAppletArgs args = {};
        UL_RC_TRY(ReadDataFromStorage(&args, sizeof(args)));
        out_start_mode = static_cast<dmi::MenuStartMode>(args.LaVersion);
        return ResultSuccess;
    }

}
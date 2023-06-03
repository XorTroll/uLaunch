#include <ul/menu/am/am_LibraryAppletUtils.hpp>
#include <ul/util/util_Scope.hpp>

namespace ul::menu::am {

    Result ReadFromInputStorage(void *out_data, const size_t data_size) {
        AppletStorage st;
        UL_RC_TRY(appletPopInData(&st));
        util::OnScopeExit st_close([&]{
            appletStorageClose(&st);
        });

        UL_RC_TRY(appletStorageRead(&st, 0, out_data, data_size));
        return ResultSuccess;
    }

    Result ReadStartMode(smi::MenuStartMode &out_start_mode) {
        LibAppletArgs args = {};
        UL_RC_TRY(ReadFromInputStorage(&args, sizeof(args)));
        out_start_mode = static_cast<smi::MenuStartMode>(args.LaVersion);
        return ResultSuccess;
    }

}
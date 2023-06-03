#include <ul/loader/loader_Input.hpp>
#include <ul/loader/loader_ProgramIdUtils.hpp>
#include <ul/loader/loader_Results.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Scope.hpp>
#include <cstring>

namespace ul::loader {

    Result ReadTargetInput(TargetInput &ipt) {
        UL_RC_TRY(appletInitialize());
        util::OnScopeExit applet_exit([]() {
            appletExit();
        });

        AppletStorage target_ipt_storage;
        if(SelfIsApplet()) {
            // We don't make use of the common args storage
            AppletStorage common_args_storage;
            UL_RC_TRY(appletPopInData(&common_args_storage));
            appletStorageClose(&common_args_storage);

            UL_RC_TRY(appletPopInData(&target_ipt_storage));
        }
        else if(SelfIsApplication()) {
            UL_RC_TRY(appletPopLaunchParameter(&target_ipt_storage, AppletLaunchParameterKind_UserChannel));
        }
        else {
            return ResultInvalidProcessType;
        }
        util::OnScopeExit close_storage([&]() {
            appletStorageClose(&target_ipt_storage);
        });

        s64 storage_size;
        UL_RC_TRY(appletStorageGetSize(&target_ipt_storage, &storage_size));
        if(static_cast<size_t>(storage_size) >= sizeof(ipt)) {
            UL_RC_TRY(appletStorageRead(&target_ipt_storage, 0, std::addressof(ipt), sizeof(ipt)));
            if(ipt.magic == TargetInput::Magic) {
                // By default, argv = <nro-path>
                if(strlen(ipt.nro_argv) == 0) {
                    strcpy(ipt.nro_argv, ipt.nro_path);
                }

                return ResultSuccess;
            }
            else {
                return ResultInvalidTargetInputMagic;
            }
        }
        else {
            return ResultInvalidTargetInputSize;
        }
    }

}
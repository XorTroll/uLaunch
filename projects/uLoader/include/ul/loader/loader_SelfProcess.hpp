
#pragma once
#include <ul/ul_Result.hpp>

namespace ul::loader {

    namespace {

        constexpr u64 SvcInfoType_MesosphereCurrentProcess = 65001;

    }

    inline Result GetSelfProcessHandle(Handle &out_proc_h) {
        u64 raw_proc_handle;
        UL_RC_TRY(svcGetInfo(&raw_proc_handle, SvcInfoType_MesosphereCurrentProcess, INVALID_HANDLE, 0));

        out_proc_h = static_cast<Handle>(raw_proc_handle);
        return ResultSuccess;
    }

    inline Result GetSelfProgramId(u64 &out_program_id) {
        return svcGetInfo(&out_program_id, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
    }

}

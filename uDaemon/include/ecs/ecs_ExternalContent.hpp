
#pragma once
#include <stratosphere.hpp>
#include <am/am_Application.hpp>
#include <am/am_LibraryApplet.hpp>

namespace ecs {

    Result RegisterExternalContent(const u64 program_id, const std::string &exefs_path);
    Result LaunchSystemProcess(const u64 program_id, const std::string &argv_str);

    inline Result RegisterLaunchAsApplet(const u64 program_id, const u32 la_version, const std::string &exefs_path, const void *args, const size_t args_size) {
        UL_RC_TRY(RegisterExternalContent(program_id, exefs_path));
        UL_RC_TRY(am::LibraryAppletStart(am::LibraryAppletGetAppletIdForProgramId(program_id), la_version, args, args_size));
        return ResultSuccess;
    }

    inline Result RegisterLaunchAsApplication(const u64 program_id, const std::string &exefs_path, const void *args, const size_t args_size, const AccountUid uid) {
        UL_RC_TRY(RegisterExternalContent(program_id, exefs_path));
        UL_RC_TRY(am::ApplicationStart(program_id, false, uid, args, args_size));
        return ResultSuccess;
    }

    inline Result RegisterLaunchAsSystemProcess(const u64 program_id, const std::string &exefs_path, const std::string &argv_str) {
        UL_RC_TRY(RegisterExternalContent(program_id, exefs_path));
        UL_RC_TRY(LaunchSystemProcess(program_id, argv_str));
        return ResultSuccess;
    }

}
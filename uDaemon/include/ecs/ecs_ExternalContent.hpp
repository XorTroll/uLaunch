
#pragma once
#include <stratosphere.hpp>
#include <am/am_Application.hpp>

namespace ecs {

    Result RegisterExternalContent(u64 program_id, const std::string &exefs_path);
    Result LaunchApplet(u64 program_id, u32 la_version, void *args, size_t args_size);
    Result LaunchSystemProcess(u64 program_id, const std::string &argv_str);

    inline Result RegisterLaunchAsApplet(u64 program_id, u32 la_version, const std::string &exefs_path, void *args, size_t args_size) {
        R_TRY(RegisterExternalContent(program_id, exefs_path));
        R_TRY(LaunchApplet(program_id, la_version, args, args_size));
        return ResultSuccess;
    }

    inline Result RegisterLaunchAsApplication(u64 program_id, const std::string &exefs_path, void *args, size_t args_size, AccountUid uid) {
        R_TRY(RegisterExternalContent(program_id, exefs_path));
        R_TRY(am::ApplicationStart(program_id, false, uid, args, args_size));
        return ResultSuccess;
    }

    inline Result RegisterLaunchAsSystemProcess(u64 program_id, const std::string &exefs_path, const std::string &argv_str) {
        R_TRY(RegisterExternalContent(program_id, exefs_path));
        R_TRY(LaunchSystemProcess(program_id, argv_str));
        return ResultSuccess;
    }

}
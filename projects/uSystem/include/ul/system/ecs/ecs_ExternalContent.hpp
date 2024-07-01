
#pragma once
#include <ul/system/la/la_LibraryApplet.hpp>
#include <ul/system/app/app_Application.hpp>
#include <ul/ul_Result.hpp>
#include <string>

namespace ul::system::ecs {

    Result RegisterExternalContent(const u64 program_id, const std::string &exefs_path);
    Result LaunchSystemProcess(const u64 program_id, const std::string &argv_str);

    inline Result RegisterLaunchAsApplet(const u64 program_id, const u32 la_version, const std::string &exefs_path, const void *args, const size_t args_size) {
        UL_RC_TRY(RegisterExternalContent(program_id, exefs_path));
        UL_RC_TRY(la::Start(la::GetAppletIdForProgramId(program_id), la_version, args, args_size));
        return ResultSuccess;
    }

    inline Result RegisterLaunchAsApplication(const u64 program_id, const std::string &exefs_path, const void *args, const size_t args_size, const AccountUid uid) {
        UL_RC_TRY(RegisterExternalContent(program_id, exefs_path));
        UL_RC_TRY(app::Start(program_id, false, uid, args, args_size));
        return ResultSuccess;
    }

    inline Result RegisterLaunchAsSystemProcess(const u64 program_id, const std::string &exefs_path, const std::string &argv_str) {
        UL_RC_TRY(RegisterExternalContent(program_id, exefs_path));
        UL_RC_TRY(LaunchSystemProcess(program_id, argv_str));
        return ResultSuccess;
    }

}


#pragma once
#include <stratosphere.hpp>
#include <ul_Include.hpp>
#include <am/am_Application.hpp>
#include <algorithm>

namespace ecs
{
    Result Initialize();
    void Exit();
    Result RegisterExternalContent(u64 program_id, std::string exefs_path);
    Result LaunchApplet(u64 program_id, u32 la_version, void *args, size_t args_size);
    Result LaunchSystemProcess(u64 program_id, std::string argv_str);

    inline Result RegisterLaunchAsApplet(u64 program_id, u32 la_version, std::string exefs_path, void *args, size_t args_size)
    {
        auto rc = RegisterExternalContent(program_id, exefs_path);
        if(R_SUCCEEDED(rc)) rc = LaunchApplet(program_id, la_version, args, args_size);
        return rc;
    }

    inline Result RegisterLaunchAsApplication(u64 program_id, std::string exefs_path, void *args, size_t args_size, AccountUid uid)
    {
        auto rc = RegisterExternalContent(program_id, exefs_path);
        if(R_SUCCEEDED(rc)) rc = am::ApplicationStart(program_id, false, uid, args, args_size);
        return rc;
    }

    inline Result RegisterLaunchAsSystemProcess(u64 program_id, std::string exefs_path, std::string argv_str)
    {
        auto rc = RegisterExternalContent(program_id, exefs_path);
        if(R_SUCCEEDED(rc)) rc = LaunchSystemProcess(program_id, argv_str);
        return rc;
    }
}
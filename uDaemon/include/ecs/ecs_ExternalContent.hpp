
#pragma once
#include <stratosphere.hpp>
#include <ul_Include.hpp>
#include <algorithm>

namespace ecs
{
    NX_CONSTEXPR AppletId DetectAppletIdByProgramId(u64 program_id)
    {
        switch(program_id)
        {
            case 0x0100000000001001:
                return AppletId_auth;
            case 0x0100000000001002:
                return AppletId_cabinet;
            case 0x0100000000001003:
                return AppletId_controller;
            case 0x0100000000001004:
                return AppletId_dataErase;
            case 0x0100000000001005:
                return AppletId_error;
            case 0x0100000000001006:
                return AppletId_netConnect;
            case 0x0100000000001007:
                return AppletId_playerSelect;
            case 0x0100000000001008:
                return AppletId_swkbd;
            case 0x0100000000001009:
                return AppletId_miiEdit;
            case 0x010000000000100A:
                return AppletId_web;
            case 0x010000000000100B:
                return AppletId_shop;
            case 0x010000000000100D:
                return AppletId_photoViewer;
            case 0x010000000000100E:
                return AppletId_set;
            case 0x010000000000100F:
                return AppletId_offlineWeb;
            case 0x0100000000001010:
                return AppletId_loginShare;
            case 0x0100000000001011:
                return AppletId_wifiWebAuth;
            case 0x0100000000001013:
                return AppletId_myPage;
        }
        return (AppletId)0;
    }

    Result Initialize();
    void Exit();
    Result RegisterExternalContent(u64 program_id, std::string exefs_path);
    Result LaunchApplet(u64 program_id, u32 la_version, void *args, size_t args_size);
    Result LaunchApplication(u64 program_id, void *args, size_t args_size, AccountUid uid);
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
        if(R_SUCCEEDED(rc)) rc = LaunchApplication(program_id, args, args_size, uid);
        return rc;
    }

    inline Result RegisterLaunchAsSystemProcess(u64 program_id, std::string exefs_path, std::string argv_str)
    {
        auto rc = RegisterExternalContent(program_id, exefs_path);
        if(R_SUCCEEDED(rc)) rc = LaunchSystemProcess(program_id, argv_str);
        return rc;
    }
}
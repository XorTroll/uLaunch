#include <hb/hb_Target.hpp>
#include <string>
#include <algorithm>

extern "C"
{
    u32 __nx_applet_type;
    u32 __nx_applet_exit_mode = 2;

    u32 __nx_fs_num_sessions = 1;
    u32 __nx_fsdev_direntry_cache_size = 1;
    bool __nx_fsdev_support_cwd = false;

    void hb_hbl_Target(const char *path, const char *argv, int do_once);
}

NX_CONSTEXPR bool IsApplet(u64 program_id)
{
    return (0x0100000000001000 <= program_id) && (program_id <= 0x0100000000001FFF);
}

NX_CONSTEXPR bool IsApplication(u64 program_id)
{
    return (0x0100000000010000 <= program_id); // (For forwarders :p) /* && (program_id <= 0x01FFFFFFFFFFFFFF); */
}

NX_CONSTEXPR bool IsSystemProcess(u64 program_id)
{
    return (0x0100000000000000 <= program_id) && (program_id <= 0x01000000000007FF);
}

NX_CONSTEXPR AppletType DetectAppletType(u64 program_id)
{
    if(IsApplet(program_id))
    {
        // OverlayApplet and SystemApplet are impossible in this case
        return AppletType_LibraryApplet;
    }
    else if(IsApplication(program_id))
    {
        // hbloader uses this instead of normal Application...
        return AppletType_SystemApplication;
    }
    return AppletType_None;
}

bool ParseTargetParams(hb::HbTargetParams &params, u64 cur_program_id, int argc, char **argv)
{
    bool ret = false;

    // Load arguments from applet storages
    if(IsApplet(cur_program_id))
    {
        // Initialize sm, initialize applet, get arguments, exit applet, exit sm
        auto rc = smInitialize();
        if(R_SUCCEEDED(rc))
        {
            rc = appletInitialize();
            if(R_SUCCEEDED(rc))
            {
                AppletStorage common_args_st;
                AppletStorage st;
                rc = appletPopInData(&common_args_st);
                if(R_SUCCEEDED(rc))
                {
                    appletStorageClose(&common_args_st);
                    rc = appletPopInData(&st);
                    if(R_SUCCEEDED(rc))
                    {
                        hb::HbTargetParams params_st = {};
                        s64 st_size = 0;
                        appletStorageGetSize(&st, &st_size);
                        if((u64)st_size >= sizeof(params_st))
                        {
                            rc = appletStorageRead(&st, 0, &params_st, sizeof(params_st));
                            if(R_SUCCEEDED(rc))
                            {
                                if(params_st.magic == UL_HB_HBTARGET_MAGIC_U32)
                                {
                                    params = params_st;
                                    ret = true;
                                }
                            }
                        }
                        appletStorageClose(&st);
                    }
                }
                appletExit();
            }
            smExit();
        }
    }
    // Load arguments from application launch parameter
    else if(IsApplication(cur_program_id))
    {
        // Initialize sm, initialize applet, get arguments, exit applet, exit sm
        auto rc = smInitialize();
        if(R_SUCCEEDED(rc))
        {
            rc = appletInitialize();
            if(R_SUCCEEDED(rc))
            {
                AppletStorage st;
                rc = appletPopLaunchParameter(&st, AppletLaunchParameterKind_UserChannel);
                if(R_SUCCEEDED(rc))
                {
                    hb::HbTargetParams params_st = {};
                    s64 st_size = 0;
                    appletStorageGetSize(&st, &st_size);
                    if((u64)st_size >= sizeof(params_st))
                    {
                        rc = appletStorageRead(&st, 0, &params_st, sizeof(params_st));
                        if(R_SUCCEEDED(rc))
                        {
                            if(params_st.magic == UL_HB_HBTARGET_MAGIC_U32)
                            {
                                params = params_st;
                                ret = true;
                            }
                        }
                    }
                    appletStorageClose(&st);
                }
                appletExit();
            }
            smExit();
        }
    }
    // Load arguments from system process argv
    else if(IsSystemProcess(cur_program_id))
    {
        // Check that 4 strings are sent
        if(argc >= 4)
        {
            std::string magic = argv[0];

            // NRO paths start with 'sdmc:/' and spaces are replaced with 0xFF
            std::string nro_path = argv[1];

            // Argv where ' ' spaces are replaced with 0xFF characters
            std::string nro_argv = argv[2];

            // This must be '0' or '1'.
            std::string target_once = argv[3];

            // Matches magic?
            if(magic == UL_HB_HBTARGET_MAGIC)
            {
                if(!nro_path.empty())
                {
                    if(!nro_argv.empty())
                    {
                        bool target_once_v = true;
                        try
                        {
                            target_once_v = (bool)std::stoi(target_once);

                            std::replace(nro_path.begin(), nro_path.end(), (char)0xFF, ' ');
                            std::replace(nro_argv.begin(), nro_argv.end(), (char)0xFF, ' ');

                            strcpy(params.nro_path, nro_path.c_str());
                            strcpy(params.nro_argv, nro_argv.c_str());
                            params.target_once = target_once_v;
                            ret = true;
                        }
                        catch(std::exception&)
                        {
                        }
                    }
                }
            }
        }
    }

    if(ret)
    {
        if(strlen(params.nro_path) == 0) ret = false;
    }
    if(ret)
    {
        if(strlen(params.nro_argv) == 0) strcpy(params.nro_argv, params.nro_path);
    }
    
    return ret;
}

int main(int argc, char **argv)
{
    u64 cur_program_id;
    auto rc = svcGetInfo(&cur_program_id, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
    if(R_SUCCEEDED(rc))
    {
        __nx_applet_type = DetectAppletType(cur_program_id);

        hb::HbTargetParams params = {};
        auto ok = ParseTargetParams(params, cur_program_id, argc, argv);
        if(ok) hb_hbl_Target(params.nro_path, params.nro_argv, (int)params.target_once);
    }

    return 0;
}
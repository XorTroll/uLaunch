#include <hb/hb_Target.hpp>
#include <string>
#include <algorithm>

extern "C" {

    u32 __nx_applet_type;
    u32 __nx_applet_exit_mode = 2;

    u32 __nx_fs_num_sessions = 1;
    u32 __nx_fsdev_direntry_cache_size = 1;
    bool __nx_fsdev_support_cwd = false;

    void hb_hbl_Target(const Handle process_handle, const char *path, const char *argv, const int do_once);

}

namespace {

    inline constexpr bool IsApplet(const u64 program_id) {
        return (0x0100000000001000 <= program_id) && (program_id <= 0x0100000000001FFF);
    }

    inline constexpr bool IsApplication(const u64 program_id) {
        return (0x0100000000010000 <= program_id);
    }

    inline constexpr bool IsSystemProcess(const u64 program_id) {
        return (0x0100000000000000 <= program_id) && (program_id <= 0x01000000000007FF);
    }

    inline constexpr AppletType DetectAppletType(const u64 program_id) {
        if(IsApplet(program_id)) {
            // OverlayApplet and SystemApplet are impossible in this case
            return AppletType_LibraryApplet;
        }
        else if(IsApplication(program_id)) {
            // hbloader uses this instead of normal Application...
            return AppletType_SystemApplication;
        }
        else {
            return AppletType_None;
        }
    }

}

namespace {

    Handle g_ProcessHandle = INVALID_HANDLE;
    u64 g_ProcessId = 0;
    u64 g_ProgramId = 0;

    void ProcessInfoReceiver(void *arg) {
        const auto session = static_cast<Handle>(reinterpret_cast<uintptr_t>(arg));
        
        auto base = armGetTls();
        hipcMakeRequestInline(base);

        s32 idx = 0;
        svcReplyAndReceive(&idx, &session, 1, INVALID_HANDLE, UINT64_MAX);

        const auto request = hipcParseRequest(base);
        g_ProcessHandle = request.data.copy_handles[0];
        g_ProcessId = request.pid;
        svcCloseHandle(session);
    }

    Result LoadProcessInfo() {
        Handle server_h;
        Handle client_h;
        // Create our own session, and close it on exit if success
        UL_RC_TRY(svcCreateSession(&server_h, &client_h, 0, 0));
        UL_ON_SCOPE_EXIT({ svcCloseHandle(client_h); });

        Thread receiver_thr;
        auto thread_arg = reinterpret_cast<void*>(static_cast<uintptr_t>(server_h));
        // Create a thread to handle our request, and close it on exit if success
        UL_RC_TRY(threadCreate(&receiver_thr, &ProcessInfoReceiver, thread_arg, nullptr, 0x1000, 0x2B, -2));
        UL_ON_SCOPE_EXIT({
            threadWaitForExit(&receiver_thr);
            threadClose(&receiver_thr);
        });

        UL_RC_TRY(threadStart(&receiver_thr));

        hipcMakeRequestInline(armGetTls(),
            .send_pid = 1,
            .num_copy_handles = 1,
        ).copy_handles[0] = CUR_PROCESS_HANDLE;

        // Will always fail, since we never actually respond to the request
        UL_RC_TRY(svcSendSyncRequest(client_h));
        return ResultSuccess;
    }

    Result LoadProgramId() {
        if(R_SUCCEEDED(svcGetInfo(&g_ProgramId, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0))) {
            return ResultSuccess;
        }

        // Let's try with pminfo
        UL_RC_TRY(pminfoInitialize());
        UL_ON_SCOPE_EXIT({ pminfoExit(); });

        UL_RC_TRY(pminfoGetProgramId(&g_ProgramId, g_ProcessId));
        return ResultSuccess;
    }

}

namespace {

    bool TryParseTargetParamsFromStorage(AppletStorage *st, hb::HbTargetParams &params) {
        // Ensure size is correct
        s64 st_size = 0;
        UL_RC_TRY(appletStorageGetSize(st, &st_size));
        hb::HbTargetParams tmp_params;
        if(static_cast<u64>(st_size) >= sizeof(tmp_params)) {
            // Read params
            UL_RC_TRY(appletStorageRead(st, 0, &tmp_params, sizeof(tmp_params)));
            if(tmp_params.magic == UL_HB_HBTARGET_MAGIC_U32) {
                params = tmp_params;
                return true;
            }
        }
        return false;
    }

    bool ParseTargetParams(const u64 cur_program_id, const int argc, char **argv, hb::HbTargetParams &out_params) {
        auto ret = false;

        // Load arguments from applet storages
        if(IsApplet(cur_program_id)) {
            // Initialize sm, initialize applet, exit sm
            UL_RC_TRY(appletInitialize());

            // Ensure applet is exited in the end
            UL_ON_SCOPE_EXIT({ appletExit(); });
            
            // We don't make use of the common args storage
            AppletStorage common_args_st;
            UL_RC_TRY(appletPopInData(&common_args_st));
            appletStorageClose(&common_args_st);

            // Get our storage
            AppletStorage hbtarget_st;
            UL_RC_TRY(appletPopInData(&hbtarget_st));
            UL_ON_SCOPE_EXIT({ appletStorageClose(&hbtarget_st); });

            // Try parse params
            ret = TryParseTargetParamsFromStorage(&hbtarget_st, out_params);
        }
        // Load arguments from application launch parameter
        else if(IsApplication(cur_program_id)) {
            // Initialize sm, initialize applet, exit sm
            UL_RC_TRY(appletInitialize());

            // Ensure applet is exited in the end
            UL_ON_SCOPE_EXIT({ appletExit(); });

            // Get our storage from user arguments
            AppletStorage hbtarget_st;
            UL_RC_TRY(appletPopLaunchParameter(&hbtarget_st, AppletLaunchParameterKind_UserChannel));
            UL_ON_SCOPE_EXIT({ appletStorageClose(&hbtarget_st); });

            // Try parse params
            ret = TryParseTargetParamsFromStorage(&hbtarget_st, out_params);
        }
        // Load arguments from system process argv
        else if(IsSystemProcess(cur_program_id)) {
            // Check that 4 strings are sent
            if(argc >= 4) {
                const std::string magic = argv[0];

                // NRO paths start with 'sdmc:/' and spaces are replaced with 0xFF
                std::string nro_path = argv[1];

                // Argv where ' ' spaces are replaced with 0xFF characters
                std::string nro_argv = argv[2];

                // This must be '0' or '1'.
                const std::string target_once = argv[3];

                // Matches magic?
                if((magic == UL_HB_HBTARGET_MAGIC) && !nro_path.empty() && !nro_argv.empty()) {
                    auto target_once_v = true;
                    try {
                        target_once_v = static_cast<bool>(std::stoi(target_once));

                        std::replace(nro_path.begin(), nro_path.end(), static_cast<char>(0xFF), ' ');
                        std::replace(nro_argv.begin(), nro_argv.end(), static_cast<char>(0xFF), ' ');

                        strcpy(out_params.nro_path, nro_path.c_str());
                        strcpy(out_params.nro_argv, nro_argv.c_str());
                        out_params.target_once = target_once_v;
                        ret = true;
                    }
                    catch(...) {}
                }
            }
        }

        if(ret) {
            if(strlen(out_params.nro_path) == 0) {
                ret = false;
            }
        }
        if(ret) {
            if(strlen(out_params.nro_argv) == 0) {
                strcpy(out_params.nro_argv, out_params.nro_path);
            }
        }
        return ret;
    }

}

int main(int argc, char **argv) {
    // Avoid checking the result (will always fail), we just care about the process handle
    LoadProcessInfo();
    if(g_ProcessHandle == INVALID_HANDLE) {
        return 0;
    }

    UL_RC_ASSERT(smInitialize());
    UL_RC_ASSERT(LoadProgramId());

    __nx_applet_type = DetectAppletType(g_ProgramId);

    hb::HbTargetParams params = {};
    const auto params_ok = ParseTargetParams(g_ProgramId, argc, argv, params);
    smExit();

    if(params_ok) {
        hb_hbl_Target(g_ProcessHandle, params.nro_path, params.nro_argv, params.target_once);
    }

    return 0;
}
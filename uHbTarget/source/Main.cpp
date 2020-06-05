#include <hb/hb_Target.hpp>
#include <string>
#include <algorithm>

extern "C" {

    u32 __nx_applet_type;
    u32 __nx_applet_exit_mode = 2;

    u32 __nx_fs_num_sessions = 1;
    u32 __nx_fsdev_direntry_cache_size = 1;
    bool __nx_fsdev_support_cwd = false;

    void hb_hbl_Target(Handle process_handle, const char *path, const char *argv, int do_once);

}

namespace {

    inline Result DoWithSmSession(std::function<Result()> fn) {
        R_TRY(smInitialize());
        R_TRY(fn());
        smExit();
        return ResultSuccess;
    }

    namespace impl {

        static Handle g_process_handle = INVALID_HANDLE;
        static u64 g_process_id = 0;
        static u64 g_program_id = 0;

        Result ProcessInfoReceiverImpl(Handle session) {
            auto base = armGetTls();
            hipcMakeRequestInline(base);

            s32 idx = 0;
            R_TRY(svcReplyAndReceive(&idx, &session, 1, INVALID_HANDLE, UINT64_MAX));

            HipcParsedRequest r = hipcParseRequest(base);
            g_process_handle = r.data.copy_handles[0];
            g_process_id = r.pid;
            svcCloseHandle(session);
            return ResultSuccess;
        }

        Result GetProgramIdImpl() {
            auto rc = svcGetInfo(&g_program_id, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
            if(R_SUCCEEDED(rc)) {
                return ResultSuccess;
            }

            // Let's try with pminfo
            R_TRY(DoWithSmSession([&]() {
                R_TRY(pminfoInitialize());
                return ResultSuccess;
            }));
            UL_ON_SCOPE_EXIT({
                pminfoExit();
            });

            R_TRY(pminfoGetProgramId(&g_program_id, impl::g_process_id));

            return ResultSuccess;
        }

        void ProcessInfoReceiver(void *arg) {
            auto session = static_cast<Handle>(reinterpret_cast<uintptr_t>(arg));
            ProcessInfoReceiverImpl(session);
        }

    }

    NX_CONSTEXPR bool IsApplet(u64 program_id) {
        return (0x0100000000001000 <= program_id) && (program_id <= 0x0100000000001FFF);
    }

    NX_CONSTEXPR bool IsApplication(u64 program_id) {
        return (0x0100000000010000 <= program_id); // (For forwarders :p) /* && (program_id <= 0x01FFFFFFFFFFFFFF); */
    }

    NX_CONSTEXPR bool IsSystemProcess(u64 program_id) {
        return (0x0100000000000000 <= program_id) && (program_id <= 0x01000000000007FF);
    }

    NX_CONSTEXPR AppletType DetectAppletType(u64 program_id) {
        if(IsApplet(program_id)) {
            // OverlayApplet and SystemApplet are impossible in this case
            return AppletType_LibraryApplet;
        }
        else if(IsApplication(program_id)) {
            // hbloader uses this instead of normal Application...
            return AppletType_SystemApplication;
        }
        return AppletType_None;
    }

    inline bool TryParseTargetParamsFromStorage(AppletStorage *st, hb::HbTargetParams &params) {
        // Ensure size is correct
        s64 st_size = 0;
        R_TRY(appletStorageGetSize(st, &st_size));
        hb::HbTargetParams tmp_params;
        if(static_cast<u64>(st_size) >= sizeof(tmp_params)) {
            // Read params
            R_TRY(appletStorageRead(st, 0, &tmp_params, sizeof(tmp_params)));
            if(tmp_params.magic == UL_HB_HBTARGET_MAGIC_U32) {
                params = tmp_params;
                return true;
            }
        }
        return false;
    }

    bool ParseTargetParams(hb::HbTargetParams &params, u64 cur_program_id, int argc, char **argv) {
        bool ret = false;

        // Load arguments from applet storages
        if(IsApplet(cur_program_id)) {
            // Initialize sm, initialize applet, exit sm
            R_TRY(DoWithSmSession([&]() {
                R_TRY(appletInitialize());
                return ResultSuccess;
            }));

            // Ensure applet is exited in the end
            UL_ON_SCOPE_EXIT({
                appletExit();
            });
            
            // We don't make use of the common args storage
            AppletStorage common_args_st;
            R_TRY(appletPopInData(&common_args_st));
            appletStorageClose(&common_args_st);

            // Get our storage
            AppletStorage hbtarget_st;
            R_TRY(appletPopInData(&hbtarget_st));
            UL_ON_SCOPE_EXIT({
                appletStorageClose(&hbtarget_st);
            });

            // Try parse params
            ret = TryParseTargetParamsFromStorage(&hbtarget_st, params);
        }
        // Load arguments from application launch parameter
        else if(IsApplication(cur_program_id)) {
            // Initialize sm, initialize applet, exit sm
            R_TRY(DoWithSmSession([&]() {
                R_TRY(appletInitialize());
                return ResultSuccess;
            }));

            // Ensure applet is exited in the end
            UL_ON_SCOPE_EXIT({
                appletExit();
            });

            // Get our storage from user arguments
            AppletStorage hbtarget_st;
            R_TRY(appletPopLaunchParameter(&hbtarget_st, AppletLaunchParameterKind_UserChannel));
            UL_ON_SCOPE_EXIT({
                appletStorageClose(&hbtarget_st);
            });

            // Try parse params
            ret = TryParseTargetParamsFromStorage(&hbtarget_st, params);
        }
        // Load arguments from system process argv
        else if(IsSystemProcess(cur_program_id)) {
            // Check that 4 strings are sent
            if(argc >= 4) {
                std::string magic = argv[0];

                // NRO paths start with 'sdmc:/' and spaces are replaced with 0xFF
                std::string nro_path = argv[1];

                // Argv where ' ' spaces are replaced with 0xFF characters
                std::string nro_argv = argv[2];

                // This must be '0' or '1'.
                std::string target_once = argv[3];

                // Matches magic?
                if(magic == UL_HB_HBTARGET_MAGIC) {
                    if(!nro_path.empty()) {
                        if(!nro_argv.empty()) {
                            bool target_once_v = true;
                            try {
                                target_once_v = (bool)std::stoi(target_once);

                                std::replace(nro_path.begin(), nro_path.end(), (char)0xFF, ' ');
                                std::replace(nro_argv.begin(), nro_argv.end(), (char)0xFF, ' ');

                                strcpy(params.nro_path, nro_path.c_str());
                                strcpy(params.nro_argv, nro_argv.c_str());
                                params.target_once = target_once_v;
                                ret = true;
                            }
                            catch(...) {}
                        }
                    }
                }
            }
        }

        if(ret) {
            if(strlen(params.nro_path) == 0) {
                ret = false;
            }
        }
        if(ret) {
            if(strlen(params.nro_argv) == 0) {
                strcpy(params.nro_argv, params.nro_path);
            }
        }
        return ret;
    }

    Result LoadProcessInfo() {
        Handle server_h;
        Handle client_h;
        // Create our own session, and close it on exit if success
        R_TRY(svcCreateSession(&server_h, &client_h, 0, 0));
        UL_ON_SCOPE_EXIT({
            svcCloseHandle(client_h);
        });

        Thread receiver_thr;
        auto thread_arg = reinterpret_cast<void*>(static_cast<uintptr_t>(server_h));
        // Create a thread to handle our request, and close it on exit if success
        R_TRY(threadCreate(&receiver_thr, &impl::ProcessInfoReceiver, thread_arg, nullptr, 0x1000, 0x2B, -2));
        UL_ON_SCOPE_EXIT({
            threadWaitForExit(&receiver_thr);
            threadClose(&receiver_thr);
        });
        R_TRY(threadStart(&receiver_thr));

        hipcMakeRequestInline(armGetTls(),
            .send_pid = 1,
            .num_copy_handles = 1,
        ).copy_handles[0] = CUR_PROCESS_HANDLE;

        R_TRY(svcSendSyncRequest(client_h));
        return ResultSuccess;
    }

    Result LoadProgramId() {
        return impl::GetProgramIdImpl();
    }

    Handle GetOwnProcessHandle() {
        return impl::g_process_handle;
    }

    u64 GetOwnProgramId() {
        return impl::g_program_id;
    }

}

int main(int argc, char **argv) {
    LoadProcessInfo();
    LoadProgramId();

    auto program_id = GetOwnProgramId();
    auto process_handle = GetOwnProcessHandle();

    __nx_applet_type = DetectAppletType(program_id);

    hb::HbTargetParams params = {};
    auto ok = ParseTargetParams(params, program_id, argc, argv);
    if(ok) {
        hb_hbl_Target(process_handle, params.nro_path, params.nro_argv, params.target_once);
    }
    return 0;
}
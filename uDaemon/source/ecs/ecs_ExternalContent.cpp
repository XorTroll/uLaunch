#include <ecs/ecs_ExternalContent.hpp>
#include <ecs/ecs_RemoteFileSystem.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include <am/am_LibraryApplet.hpp>
#include <am/am_HomeMenu.hpp>

namespace ecs {

    namespace {

        // fsp-srv's options, since we're hosting FileSystem sessions
        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x800;
            static constexpr size_t MaxDomains = 0x40;
            static constexpr size_t MaxDomainObjects = 0x4000;
        };

        constexpr size_t MaxServers = 0;

        bool initialized = false;
        Thread manager_process_thread;
        ams::sf::hipc::ServerManager<MaxServers, ServerOptions> manager_instance;

    }

    static void ECSManagerThread(void *arg) {
        manager_instance.LoopProcess();
    }

    Result Initialize() {
        if(initialized) {
            return ResultSuccess;
        }
        R_TRY(ldrShellInitialize());
        R_TRY(pmshellInitialize());
        
        R_TRY(threadCreate(&manager_process_thread, &ECSManagerThread, nullptr, nullptr, 0x8000, 0x2b, -2));
        R_TRY(threadStart(&manager_process_thread));
        return ResultSuccess;
    }

    void Exit() {
        threadWaitForExit(&manager_process_thread);
        pmshellExit();
        ldrShellExit();
    }

    static inline Result ldrShellAtmosphereRegisterExternalCode(u64 app_id, Handle *out_h) {
        return serviceDispatchIn(ldrShellGetServiceSession(), 65000, app_id,
            .out_handle_attrs = { SfOutHandleAttr_HipcMove },
            .out_handles = out_h,
        );
    }

    static inline Result ldrShellAtmosphereUnregisterExternalCode(u64 app_id) {
        return serviceDispatchIn(ldrShellGetServiceSession(), 65001, app_id);
    }

    Result RegisterExternalContent(u64 app_id, const std::string &exefs_path) {
        Handle move_h = INVALID_HANDLE;
        ldrShellAtmosphereUnregisterExternalCode(app_id);
        R_TRY(ldrShellAtmosphereRegisterExternalCode(app_id, &move_h));

        // Create a remote access session to SD's filesystem (ams's original remote fs would close it, and we don't want that!)
        std::unique_ptr<ams::fs::fsa::IFileSystem> sd_ifs = std::make_unique<RemoteSdCardFileSystem>();
        auto sd_ifs_ipc = std::make_shared<ams::fssrv::impl::FileSystemInterfaceAdapter>(std::make_shared<ams::fssystem::SubDirectoryFileSystem>(std::move(sd_ifs), exefs_path.c_str()), false);

        ams::sf::cmif::ServiceObjectHolder srv_holder(std::move(sd_ifs_ipc));
        R_TRY(manager_instance.RegisterSession(move_h, std::move(srv_holder)).GetValue());

        return ResultSuccess;
    }

    Result LaunchApplet(u64 program_id, u32 la_version, void *args, size_t args_size) {
        auto appletid = am::LibraryAppletGetAppletIdForProgramId(program_id);
        if(appletid == 0) {
            return 0xDEAD;
        }
        R_TRY(am::LibraryAppletStart(appletid, la_version, args, args_size));
        return ResultSuccess;
    }
    
    Result LaunchSystemProcess(u64 program_id, const std::string &argv_str) {
        R_TRY(ldrShellSetProgramArguments(program_id, argv_str.c_str(), argv_str.length()));
        NcmProgramLocation loc = {
            .program_id = program_id,
            .storageID = NcmStorageId_BuiltInSystem,
        };
        u64 pid;
        R_TRY(pmshellLaunchProgram(0, &loc, &pid));
        return ResultSuccess;
    }

}
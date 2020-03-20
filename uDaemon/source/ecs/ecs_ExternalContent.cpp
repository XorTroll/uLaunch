#include <ecs/ecs_ExternalContent.hpp>
#include <ecs/ecs_RemoteFileSystem.hpp>
#include <map>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include <am/am_Application.hpp>
#include <am/am_LibraryApplet.hpp>
#include <am/am_HomeMenu.hpp>
#include <am/am_DaemonMenuInteraction.hpp>

namespace ecs
{
    namespace
    {
        struct ServerOptions
        {
            static constexpr size_t PointerBufferSize = 0x800;
            static constexpr size_t MaxDomains = 0x40;
            static constexpr size_t MaxDomainObjects = 0x4000;
        };

        constexpr size_t MaxServers = 0;

        bool initialized = false;
        Thread manager_process_thread;
        ams::sf::hipc::ServerManager<MaxServers, ServerOptions> manager_instance;
    }

    static void ECSManagerThread(void *arg)
    {
        manager_instance.LoopProcess();
    }

    Result Initialize()
    {
        if(initialized) return 0;
        auto rc = ldrShellInitialize();
        if(R_SUCCEEDED(rc)) rc = pmshellInitialize();
        initialized = R_SUCCEEDED(rc);
        if(initialized)
        {
            R_TRY(threadCreate(&manager_process_thread, &ECSManagerThread, nullptr, nullptr, 0x8000, 0x2b, -2));
            R_TRY(threadStart(&manager_process_thread));
        }
        return rc;
    }

    void Exit()
    {
        if(initialized)
        {
            threadWaitForExit(&manager_process_thread);
            pmshellExit();
            ldrShellExit();
            initialized = false;
        }
    }

    static inline Result ldrShellAtmosphereRegisterExternalCode(u64 app_id, Handle *out_h)
    {
        return serviceDispatchIn(ldrShellGetServiceSession(), 65000, app_id,
            .out_handle_attrs = { SfOutHandleAttr_HipcMove },
            .out_handles = out_h,
        );
    }

    static inline Result ldrShellAtmosphereUnregisterExternalCode(u64 app_id)
    {
        return serviceDispatchIn(ldrShellGetServiceSession(), 65001, app_id);
    }

    Result RegisterExternalContent(u64 app_id, std::string exefs_path)
    {
        Handle move_h = INVALID_HANDLE;
        ldrShellAtmosphereUnregisterExternalCode(app_id);
        R_TRY(ldrShellAtmosphereRegisterExternalCode(app_id, &move_h));

        // Create a remote access session to SD's filesystem (ams's original remote fs would close it, and we don't want that!)
        std::unique_ptr<ams::fs::fsa::IFileSystem> sd_ifs = std::make_unique<RemoteSdCardFileSystem>();
        auto sd_ifs_ipc = std::make_shared<ams::fssrv::impl::FileSystemInterfaceAdapter>(std::make_shared<ams::fssystem::SubDirectoryFileSystem>(std::move(sd_ifs), exefs_path.c_str()), false);

        ams::sf::cmif::ServiceObjectHolder srv_holder(std::move(sd_ifs_ipc));
        R_TRY(manager_instance.RegisterSession(move_h, std::move(srv_holder)).GetValue());

        return 0;
    }

    Result LaunchApplet(u64 program_id, u32 la_version, void *args, size_t args_size)
    {
        Result rc = 0xdead;
        auto appletid = am::LibraryAppletGetAppletIdForProgramId(program_id);
        if(appletid != 0) rc = am::LibraryAppletStart(appletid, la_version, args, args_size);
        return rc;
    }
    
    Result LaunchSystemProcess(u64 program_id, std::string argv_str)
    {
        auto rc = ldrShellSetProgramArguments(program_id, argv_str.c_str(), argv_str.length());
        if(R_SUCCEEDED(rc))
        {
            NcmProgramLocation loc = {
                .program_id = program_id,
                .storageID = NcmStorageId_BuiltInSystem,
            };
            u64 pid;
            rc = pmshellLaunchProgram(0, &loc, &pid);
        }
        return rc;
    }
}
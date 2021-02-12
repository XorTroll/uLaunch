#include <ecs/ecs_ExternalContent.hpp>
#include <ipc/ipc_GlobalManager.hpp>
#include <am/am_LibraryApplet.hpp>
#include <am/am_HomeMenu.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

namespace {

    inline Result ldrShellAtmosphereRegisterExternalCode(u64 app_id, Handle *out_h) {
        return serviceDispatchIn(ldrShellGetServiceSession(), 65000, app_id,
            .out_handle_attrs = { SfOutHandleAttr_HipcMove },
            .out_handles = out_h,
        );
    }

}

namespace ecs {

    Result RegisterExternalContent(u64 app_id, const std::string &exefs_path) {
        auto move_h = INVALID_HANDLE;
        R_TRY(ldrShellAtmosphereRegisterExternalCode(app_id, &move_h));

        FsFileSystem sd_fs;
        R_TRY(fsOpenSdCardFileSystem(&sd_fs));
        std::unique_ptr<ams::fs::fsa::IFileSystem> remote_sd_fs = std::make_unique<ams::fs::RemoteFileSystem>(sd_fs);
        auto subdir_fs = std::make_shared<ams::fssystem::SubDirectoryFileSystem>(std::move(remote_sd_fs), exefs_path.c_str());
        auto sd_ifs_ipc = ipc::MakeShared<ams::fssrv::sf::IFileSystem, ams::fssrv::impl::FileSystemInterfaceAdapter>(std::move(subdir_fs), false);

        R_TRY(ipc::GetGlobalManager().RegisterSession(move_h, ams::sf::cmif::ServiceObjectHolder(std::move(sd_ifs_ipc))).GetValue());

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
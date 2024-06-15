#include <ul/system/ecs/ecs_ExternalContent.hpp>
#include <ul/system/sf/sf_IpcManager.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

namespace ul::system::ecs {

    namespace {

        inline Result ldrShellAtmosphereRegisterExternalCode(const u64 app_id, Handle *out_h) {
            return serviceDispatchIn(ldrShellGetServiceSession(), 65000, app_id,
                .out_handle_attrs = { SfOutHandleAttr_HipcMove },
                .out_handles = out_h,
            );
        }

    }

    Result RegisterExternalContent(const u64 program_id, const std::string &exefs_path) {
        auto move_h = INVALID_HANDLE;
        UL_RC_TRY(ldrShellAtmosphereRegisterExternalCode(program_id, &move_h));

        FsFileSystem sd_fs;
        UL_RC_TRY(fsOpenSdCardFileSystem(&sd_fs));
        std::shared_ptr<::ams::fs::fsa::IFileSystem> remote_sd_fs = std::make_shared<::ams::fs::RemoteFileSystem>(sd_fs);
        auto subdir_fs = std::make_shared<::ams::fssystem::SubDirectoryFileSystem>(std::move(remote_sd_fs));
        ::ams::fs::Path exefs_fs_path;
        UL_RC_TRY(exefs_fs_path.Initialize(exefs_path.c_str(), exefs_path.length()));
        UL_RC_TRY(exefs_fs_path.Normalize(::ams::fs::PathFlags{}));
        UL_RC_TRY(subdir_fs->Initialize(exefs_fs_path));

        auto sd_ifs_ipc = sf::MakeShared<::ams::fssrv::sf::IFileSystem, ::ams::fssrv::impl::FileSystemInterfaceAdapter>(std::move(subdir_fs), false);
        UL_RC_TRY(sf::RegisterSession(move_h, ::ams::sf::cmif::ServiceObjectHolder(std::move(sd_ifs_ipc))));
        return ResultSuccess;
    }

    Result LaunchSystemProcess(const u64 program_id, const std::string &argv_str) {
        UL_RC_TRY(ldrShellSetProgramArguments(program_id, argv_str.c_str(), argv_str.length()));
        NcmProgramLocation loc = {
            .program_id = program_id,
            .storageID = NcmStorageId_BuiltInSystem
        };

        u64 pid;
        UL_RC_TRY(pmshellLaunchProgram(0, &loc, &pid));
        return ResultSuccess;
    }

}
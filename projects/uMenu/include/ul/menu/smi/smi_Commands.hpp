
#pragma once
#include <ul/loader/loader_TargetInput.hpp>
#include <ul/menu/smi/smi_MenuProtocol.hpp>

namespace ul::menu::smi {

    inline Result SetSelectedUser(const AccountUid &user_id) {
        return SendCommand(SystemMessage::SetSelectedUser,
            [&](ScopedStorageWriter &writer) {
                writer.Push(user_id);
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }
    
    inline Result LaunchApplication(const u64 app_id) {
        return SendCommand(SystemMessage::LaunchApplication,
            [&](ScopedStorageWriter &writer) {
                writer.Push(app_id);
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

    inline Result ResumeApplication() {
        return SendCommand(SystemMessage::ResumeApplication,
            [&](ScopedStorageWriter &writer) {
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

    inline Result TerminateApplication() {
        return SendCommand(SystemMessage::TerminateApplication,
            [&](ScopedStorageWriter &writer) {
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

    inline Result LaunchHomebrewLibraryApplet(const std::string &nro_path, const std::string &nro_argv) {
        const auto target_ipt = loader::TargetInput::Create(nro_path, nro_argv, false);

        return SendCommand(SystemMessage::LaunchHomebrewLibraryApplet,
            [&](ScopedStorageWriter &writer) {
                writer.Push(target_ipt);
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

    inline Result LaunchHomebrewApplication(const std::string &nro_path, const std::string &nro_argv) {
        const auto target_ipt = loader::TargetInput::Create(nro_path, nro_argv, false);

        return SendCommand(SystemMessage::LaunchHomebrewApplication,
            [&](ScopedStorageWriter &writer) {
                writer.Push(target_ipt);
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

    inline Result OpenWebPage(const char(&url)[500]) {
        return SendCommand(SystemMessage::OpenWebPage,
            [&](ScopedStorageWriter &writer) {
                writer.PushData(url, sizeof(url));
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

    inline Result OpenAlbum() {
        return SendCommand(SystemMessage::OpenAlbum,
            [&](ScopedStorageWriter &writer) {
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

    inline Result SetHomebrewTakeoverApplication(const u64 app_id) {
        return SendCommand(SystemMessage::SetHomebrewTakeoverApplication,
            [&](ScopedStorageWriter &writer) {
                writer.Push(app_id);
                return ResultSuccess;
            },
            [](ScopedStorageReader &reader) {
                // ...
                return ResultSuccess;
            }
        );
    }

}
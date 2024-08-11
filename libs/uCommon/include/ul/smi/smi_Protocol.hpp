
#pragma once
#include <ul/loader/loader_TargetTypes.hpp>
#include <ul/ul_Result.hpp>
#include <functional>

namespace ul::smi {

    enum class MenuStartMode : u32 {
        Invalid,
        Start,
        MainMenu,
        MainMenuApplicationSuspended,
        SettingsMenu
    };

    enum class MenuMessage : u32 {
        Invalid,
        HomeRequest,
        SdCardEjected,
        GameCardMountFailure,
        PreviousLaunchFailure,
        ChosenHomebrew,
        FinishedSleep,
        ApplicationRecordsChanged,
        ApplicationVerifyProgress,
        ApplicationVerifyResult
    };

    struct MenuMessageContext {
        MenuMessage msg;

        union {
            struct {
                Result mount_rc;
            } gc_mount_failure;
            struct {
                char nro_path[FS_MAX_PATH];
            } chosen_hb;
            struct {
                bool records_added_or_deleted;
            } app_records_changed;
            struct {
                u64 app_id;
                u64 done;
                u64 total;
            } app_verify_progress;
            struct {
                u64 app_id;
                Result rc;
                Result detail_rc;
            } app_verify_rc;
        };
    };

    enum class SystemMessage : u32 {
        Invalid,
        SetSelectedUser,
        LaunchApplication,
        ResumeApplication,
        TerminateApplication,
        LaunchHomebrewLibraryApplet,
        LaunchHomebrewApplication,
        ChooseHomebrew,
        OpenWebPage,
        OpenAlbum,
        RestartMenu,
        ReloadConfig,
        UpdateMenuPaths,
        UpdateMenuIndex,
        OpenUserPage,
        OpenMiiEdit,
        OpenAddUser,
        OpenNetConnect,
        ListAddedApplications,
        ListDeletedApplications,
        OpenCabinet,
        StartVerifyApplication,
        ListInVerifyApplications
    };

    struct SystemStatus {
        AccountUid selected_user;
        loader::TargetInput suspended_hb_target_ipt; // Set if homebrew (launched as an application) is currently suspended
        u64 suspended_app_id; // Set if any normal application is suspended
        char last_menu_fs_path[FS_MAX_PATH];
        char last_menu_path[FS_MAX_PATH];
        u32 last_menu_index;
        bool reload_theme_cache;
        u32 last_added_app_count;
        u32 last_deleted_app_count;
        u32 in_verify_app_count;
    };

    using CommandFunction = Result(*)(void*, const size_t, const bool);

    struct CommandCommonHeader {
        u32 magic;
        u32 val;
    };

    constexpr u32 CommandMagic = 0x21494D53; // "SMI!"
    constexpr size_t CommandStorageSize = 0x8000;

    namespace impl {

        using PopStorageFunction = Result(*)(AppletStorage*, const bool);
        using PushStorageFunction = Result(*)(AppletStorage*);

        template<PushStorageFunction PushStorageFn>
        class ScopedStorageWriterBase {
            protected:
                AppletStorage st;
                size_t cur_offset;

            public:
                ScopedStorageWriterBase() : st(), cur_offset(0) {}

                ~ScopedStorageWriterBase() {
                    UL_RC_ASSERT(PushStorage(&this->st));
                    appletStorageClose(&this->st);
                }

                static inline Result PushStorage(AppletStorage *st) {
                    return PushStorageFn(st);
                }

                inline void Initialize(const AppletStorage &st) {
                    this->st = st;
                }

                inline Result PushData(const void *data, const size_t size) {
                    if((cur_offset + size) <= CommandStorageSize) {
                        UL_RC_TRY(appletStorageWrite(&this->st, this->cur_offset, data, size));
                        this->cur_offset += size;
                        return ResultSuccess;
                    }
                    else {
                        return ResultOutOfPushSpace;
                    }
                }

                template<typename T>
                inline Result Push(const T t) {
                    return this->PushData(&t, sizeof(T));
                }
        };

        template<PopStorageFunction PopStorageFn>
        class ScopedStorageReaderBase {
            protected:
                AppletStorage st;
                size_t cur_offset;

            public:
                ScopedStorageReaderBase() : st(), cur_offset(0) {}

                ~ScopedStorageReaderBase() {
                    appletStorageClose(&this->st);
                }

                static inline Result PopStorage(AppletStorage *st, const bool wait) {
                    return PopStorageFn(st, wait);
                }

                inline void Initialize(const AppletStorage &st) {
                    this->st = st;
                }

                inline Result PopData(void *out_data, const size_t size) {
                    if((cur_offset + size) <= CommandStorageSize) {
                        UL_RC_TRY(appletStorageRead(&this->st, this->cur_offset, out_data, size));
                        this->cur_offset += size;
                        return ResultSuccess;
                    }
                    else {
                        return ResultOutOfPopSpace;
                    }
                }
                
                template<typename T>
                inline Result Pop(T &out_t) {
                    return this->PopData(std::addressof(out_t), sizeof(T));
                }
        };

        template<typename StorageReader>
        inline Result OpenStorageReader(StorageReader &reader, const bool wait) {
            AppletStorage st = {};
            UL_RC_TRY(StorageReader::PopStorage(&st, wait));

            reader.Initialize(st);
            return ResultSuccess;
        }

        template<typename StorageWriter>
        inline Result OpenStorageWriter(StorageWriter &writer) {
            AppletStorage st = {};
            UL_RC_TRY(appletCreateStorage(&st, CommandStorageSize));
            
            writer.Initialize(st);
            return ResultSuccess;
        }

        template<typename StorageWriter, typename StorageReader, typename MessageType>
        inline Result SendCommandImpl(const MessageType msg_type, std::function<Result(StorageWriter&)> push_fn, std::function<Result(StorageReader&)> pop_fn) {
            {
                const CommandCommonHeader in_header = {
                    .magic = CommandMagic,
                    .val = static_cast<u32>(msg_type)
                };

                StorageWriter writer;
                UL_RC_TRY(OpenStorageWriter(writer));
                UL_RC_TRY(writer.Push(in_header));

                UL_RC_TRY(push_fn(writer));
            }

            {
                CommandCommonHeader out_header = {};

                StorageReader reader;
                UL_RC_TRY(OpenStorageReader(reader, true));
                UL_RC_TRY(reader.Pop(out_header));
                if(out_header.magic != CommandMagic) {
                    return ResultInvalidOutHeaderMagic;
                }

                UL_RC_TRY(out_header.val);

                UL_RC_TRY(pop_fn(reader));
            }

            return ResultSuccess;
        }

        template<typename StorageWriter, typename StorageReader, typename MessageType>
        inline Result ReceiveCommandImpl(std::function<Result(const MessageType, StorageReader&)> pop_fn, std::function<Result(const MessageType, StorageWriter&)> push_fn) {
            CommandCommonHeader in_out_header = {};
            auto msg_type = MessageType();

            {
                StorageReader reader;
                UL_RC_TRY(OpenStorageReader(reader, false));
                UL_RC_TRY(reader.Pop(in_out_header));
                if(in_out_header.magic != CommandMagic) {
                    return ResultInvalidInHeaderMagic;
                }

                msg_type = static_cast<MessageType>(in_out_header.val);
                in_out_header.val = pop_fn(msg_type, reader);
            }

            {
                StorageWriter writer;
                UL_RC_TRY(OpenStorageWriter(writer));
                UL_RC_TRY(writer.Push(in_out_header));

                if(R_SUCCEEDED(in_out_header.val)) {
                    UL_RC_TRY(push_fn(msg_type, writer));
                }
            }

            return ResultSuccess;
        }

        using StorageFunction = Result(*)(AppletStorage*);

        Result LoopWaitStorageFunctionImpl(StorageFunction st_fn, AppletStorage *st, const bool wait);

    }

}

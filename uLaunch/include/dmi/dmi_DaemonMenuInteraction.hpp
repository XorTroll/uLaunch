
#pragma once
#include <ul_Include.hpp>
#include <hb/hb_Target.hpp>

#define AM_DAEMON_PRIVATE_SERVICE_NAME "qdmnsrv"
#define AM_DAEMON_PUBLIC_SERVICE_NAME "ulaunch"

namespace dmi {

    enum class MenuStartMode {
        Invalid,
        StartupScreen,
        Menu,
        MenuApplicationSuspended,
        MenuLaunchFailure
    };

    enum class MenuMessage : u32 {
        Invalid,
        HomeRequest
    };

    enum class DaemonMessage {
        Invalid,
        SetSelectedUser,
        LaunchApplication,
        ResumeApplication,
        TerminateApplication,
        LaunchHomebrewLibraryApplet,
        LaunchHomebrewApplication,
        OpenWebPage,
        OpenAlbum,
        RestartMenu,
    };

    struct DaemonStatus {
        AccountUid selected_user;
        hb::HbTargetParams params; // Set if homebrew (via flog takeover) is suspended
        u64 app_id; // Set if any title (other than flog) is suspended
    };

    using CommandFunction = Result(*)(void*, size_t, bool);

    struct CommandCommonHeader {
        u32 magic;
        u32 val;
    };

    constexpr u32 CommandMagic = 0x434D4151;
    constexpr size_t CommandStorageSize = 0x800;

    namespace impl {

        using PopStorageFunction = Result(*)(AppletStorage*, bool);
        using PushStorageFunction = Result(*)(AppletStorage*);

        template<PushStorageFunction PushStorageFn>
        class ScopedStorageWriterBase {

            protected:
                AppletStorage st;
                size_t cur_offset;

            public:
                ScopedStorageWriterBase() : st({}), cur_offset(0) {}

                ~ScopedStorageWriterBase() {
                    UL_ASSERT(this->PushStorage(&this->st));
                    appletStorageClose(&this->st);
                }

                Result PushStorage(AppletStorage *st) {
                    return PushStorageFn(st);
                }

                void Initialize(AppletStorage st) {
                    this->st = st;
                }

                Result PushData(void *data, size_t size) {
                    if((cur_offset + size) <= CommandStorageSize) {
                        R_TRY(appletStorageWrite(&this->st, this->cur_offset, data, size));
                        this->cur_offset += size;
                        return ResultSuccess;
                    }
                    return 0xBAFF;
                }

                template<typename T>
                Result Push(T t) {
                    return PushData(&t, sizeof(T));
                }

        };

        template<PopStorageFunction PopStorageFn>
        class ScopedStorageReaderBase {

            protected:
                AppletStorage st;
                size_t cur_offset;

            public:
                ScopedStorageReaderBase() : st({}), cur_offset(0) {}

                ~ScopedStorageReaderBase() {
                    appletStorageClose(&this->st);
                }

                Result PopStorage(AppletStorage *st, bool wait) {
                    return PopStorageFn(st, wait);
                }

                void Initialize(AppletStorage st) {
                    this->st = st;
                }

                Result PopData(void *out_data, size_t size) {
                    if((cur_offset + size) <= CommandStorageSize) {
                        R_TRY(appletStorageRead(&this->st, this->cur_offset, out_data, size));
                        this->cur_offset += size;
                        return ResultSuccess;
                    }
                    return 0xBAFF;
                }
                
                template<typename T>
                Result Pop(T &out_t) {
                    return PopData(&out_t, sizeof(T));
                }

        };

        template<typename StorageReader>
        inline Result OpenStorageReader(StorageReader &reader, bool wait) {
            AppletStorage st = {};
            R_TRY(reader.PopStorage(&st, wait));

            reader.Initialize(st);
            return ResultSuccess;
        }

        template<typename StorageWriter>
        inline Result OpenStorageWriter(StorageWriter &writer) {
            AppletStorage st = {};
            R_TRY(appletCreateStorage(&st, CommandStorageSize));
            
            writer.Initialize(st);
            return ResultSuccess;
        }

        template<typename StorageWriter, typename StorageReader, typename MessageType>
        Result SendCommandImpl(MessageType msg_type, std::function<Result(StorageWriter&)> push_fn, std::function<Result(StorageReader&)> pop_fn) {
            CommandCommonHeader header = { CommandMagic, static_cast<u32>(msg_type) };
            {
                StorageWriter writer;
                R_TRY(OpenStorageWriter(writer));
                R_TRY(writer.Push(header));

                R_TRY(push_fn(writer));
            }

            {
                StorageReader reader;
                R_TRY(OpenStorageReader(reader, true));
                R_TRY(reader.Pop(header));
                if(header.magic != CommandMagic) {
                    return 0xBAFA;
                }
                R_TRY(static_cast<Result>(header.val));

                R_TRY(pop_fn(reader));
            }

            return ResultSuccess;
        }

        template<typename StorageWriter, typename StorageReader, typename MessageType>
        Result ReceiveCommandImpl(std::function<Result(MessageType, StorageReader&)> pop_fn, std::function<Result(MessageType, StorageWriter&)> push_fn) {
            CommandCommonHeader header = {};
            auto msg_type = MessageType();
            {
                StorageReader reader;
                R_TRY(OpenStorageReader(reader, false));
                R_TRY(reader.Pop(header));
                if(header.magic != CommandMagic) {
                    return 0xBAFA;
                }
                msg_type = static_cast<MessageType>(header.val);

                header.val = pop_fn(msg_type, reader);
            }

            {
                StorageWriter writer;
                R_TRY(OpenStorageWriter(writer));
                R_TRY(writer.Push(header));

                if(R_SUCCEEDED(header.val)) {
                    R_TRY(push_fn(msg_type, writer));
                }
            }

            return ResultSuccess;
        }

    }

    namespace daemon {

        Result PopStorage(AppletStorage *st, bool wait);
        Result PushStorage(AppletStorage *st);

        using DaemonScopedStorageReader = impl::ScopedStorageReaderBase<&PopStorage>;

        using DaemonScopedStorageWriter = impl::ScopedStorageWriterBase<&PushStorage>;

        inline Result SendCommand(MenuMessage msg, std::function<Result(DaemonScopedStorageWriter&)> push_fn, std::function<Result(DaemonScopedStorageReader&)> pop_fn) {
            return impl::SendCommandImpl(msg, push_fn, pop_fn);
        }

        inline Result ReceiveCommand(std::function<Result(DaemonMessage, DaemonScopedStorageReader&)> pop_fn, std::function<Result(DaemonMessage, DaemonScopedStorageWriter&)> push_fn) {
            return impl::ReceiveCommandImpl(pop_fn, push_fn);
        }

    }

    namespace menu {

        Result PopStorage(AppletStorage *st, bool wait);
        Result PushStorage(AppletStorage *st);

        using MenuScopedStorageReader = impl::ScopedStorageReaderBase<&PopStorage>;

        using MenuScopedStorageWriter = impl::ScopedStorageWriterBase<&PushStorage>;

        inline Result SendCommand(DaemonMessage msg, std::function<Result(MenuScopedStorageWriter&)> push_fn, std::function<Result(MenuScopedStorageReader&)> pop_fn) {
            return impl::SendCommandImpl(msg, push_fn, pop_fn);
        }

        inline Result ReceiveCommand(std::function<Result(MenuMessage, MenuScopedStorageReader&)> pop_fn, std::function<Result(MenuMessage, MenuScopedStorageWriter&)> push_fn) {
            return impl::ReceiveCommandImpl(pop_fn, push_fn);
        }

    }

}
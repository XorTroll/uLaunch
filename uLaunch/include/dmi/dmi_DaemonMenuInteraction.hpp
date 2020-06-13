
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

    enum class MenuMessage {
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
        GetSelectedUser,
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

    static constexpr u32 Magic = 0x434D4151;
    static constexpr size_t BlockSize = 0x800;

    namespace impl {

        Result DaemonWriteImpl(void *data, size_t size, bool wait);
        Result DaemonReadImpl(void *data, size_t size, bool wait);

        Result MenuWriteImpl(void *data, size_t size, bool wait);
        Result MenuReadImpl(void *data, size_t size, bool wait);

    }

    template<CommandFunction WriteFn, typename V, bool Wait>
    class CommandWriter {
        static_assert(sizeof(V) == sizeof(u32), "Invalid value type");

        private:
            CommandCommonHeader request;
            u8 *data_block;
            size_t data_pos;
            Result inner_rc;
            bool write_done;

        public:
            CommandWriter(V value) : request({ Magic, static_cast<u32>(value) }), data_block(new u8[BlockSize]()), data_pos(0), inner_rc(ResultSuccess), write_done(false) {
                this->inner_rc = WriteFn(&this->request, sizeof(this->request), Wait);
            }

            inline void FinishWrite() {
                if(!this->write_done) {
                    WriteFn(this->data_block, BlockSize, Wait);
                    if(this->data_block != nullptr) {
                        delete[] this->data_block;
                        this->data_block = nullptr;
                    }
                    this->write_done = true;
                }
            }

            inline Result GetResult() {
                return this->inner_rc;
            }

            inline operator bool() {
                return R_SUCCEEDED(this->inner_rc);
            }

            template<typename T>
            inline void Write(T t) {
                if(!this->write_done) {
                    if((this->data_pos + sizeof(T)) <= BlockSize) {
                        memcpy(this->data_block + this->data_pos, &t, sizeof(T));
                        this->data_pos += sizeof(T);
                    }
                }
            }

    };

    template<CommandFunction ReadFn, typename V, bool Wait>
    class CommandReader {
        static_assert(sizeof(V) == sizeof(u32), "Invalid value type");

        private:
            CommandCommonHeader response;
            u8 *data_block;
            size_t data_pos;
            Result inner_rc;
            bool read_done;

        public:
            CommandReader() : response(), data_block(new u8[BlockSize]()), data_pos(0), inner_rc(ResultSuccess), read_done(false) {
                this->inner_rc = ReadFn(&this->response, sizeof(this->response), Wait);
                if(R_SUCCEEDED(this->inner_rc)) {
                    this->inner_rc = ReadFn(this->data_block, BlockSize, Wait);
                }
            }

            inline void FinishRead() {
                if(!this->read_done) {
                    if(this->data_block != nullptr) {
                        delete[] this->data_block;
                        this->data_block = nullptr;
                    }
                    this->read_done = true;
                }
            }

            inline V GetValue() {
                return static_cast<V>(this->response.val);
            }

            inline Result GetResult() {
                return this->inner_rc;
            }

            inline operator bool() {
                return R_SUCCEEDED(this->inner_rc);
            }

            template<typename T>
            inline T Read() {
                if(!this->read_done) {
                    if((this->data_pos + sizeof(T)) <= BlockSize) {
                        auto t = *reinterpret_cast<T*>(this->data_block + this->data_pos);
                        data_pos += sizeof(T);
                        return t;
                    }
                }
                return T();
            }

    };

    template<typename V, bool Wait>
    using DaemonReader = CommandReader<&impl::DaemonReadImpl, V, Wait>;

    template<typename V, bool Wait>
    using DaemonWriter = CommandWriter<&impl::DaemonWriteImpl, V, Wait>;

    using DaemonMessageReader = DaemonReader<DaemonMessage, false>;
    using DaemonMessageWriter = DaemonWriter<MenuMessage, false>;

    using DaemonResultReader = DaemonReader<Result, true>;
    using DaemonResultWriter = DaemonWriter<Result, false>;

    template<typename V, bool Wait>
    using MenuReader = CommandReader<&impl::MenuReadImpl, V, Wait>;

    template<typename V, bool Wait>
    using MenuWriter = CommandWriter<&impl::MenuWriteImpl, V, Wait>;

    using MenuMessageReader = MenuReader<MenuMessage, false>;
    using MenuMessageWriter = MenuWriter<DaemonMessage, false>;

    using MenuResultReader = MenuReader<Result, true>;
    using MenuResultWriter = MenuWriter<Result, false>;

}
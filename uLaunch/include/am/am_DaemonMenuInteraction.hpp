
#pragma once
#include <ul_Include.hpp>
#include <hb/hb_Target.hpp>

#define AM_DAEMON_PRIVATE_SERVICE_NAME "qdmnsrv"
#define AM_DAEMON_PUBLIC_SERVICE_NAME "ulaunch"

namespace am
{
    enum class MenuStartMode
    {
        Invalid,
        StartupScreen,
        Menu,
        MenuApplicationSuspended,
        MenuLaunchFailure
    };

    enum class MenuMessage
    {
        Invalid,
        HomeRequest
    };

    enum class DaemonMessage
    {
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
    };

    struct DaemonStatus
    {
        AccountUid selected_user;
        hb::HbTargetParams params; // Set if homebrew (via flog takeover) is suspended
        u64 app_id; // Set if any title (other than flog) is suspended
    };

    ResultWith<MenuStartMode> Menu_ProcessInput();

    Result Daemon_MenuWriteImpl(void *data, size_t size, bool wait);
    Result Daemon_MenuReadImpl(void *data, size_t size, bool wait);

    Result Menu_DaemonWriteImpl(void *data, size_t size, bool wait);
    Result Menu_DaemonReadImpl(void *data, size_t size, bool wait);

    typedef Result(*DMCommandRWFunction)(void*, size_t, bool);

    struct DMCommandCommonHeader
    {
        u32 magic;
        u32 val;
    };

    static constexpr u32 Magic = 0x434D4151;
    static constexpr size_t BlockSize = 0x4000;

    template<DMCommandRWFunction WriteFn>
    class DMCommandWriter
    {
        private:
            DMCommandCommonHeader request;
            u8 *data_block;
            size_t data_pos;
            Result inner_rc;
            bool write_done;

        public:
            DMCommandWriter(u32 value)
            {
                write_done = false;
                request.magic = Magic;
                request.val = value;
                data_pos = 0;
                data_block = new u8[BlockSize]();
                inner_rc = WriteFn(&request, sizeof(request), false);
            }

            ~DMCommandWriter()
            {
                FinishWrite();
            }

            void FinishWrite()
            {
                if(!write_done)
                {
                    WriteFn(data_block, BlockSize, false);
                    if(data_block) delete[] data_block;
                    write_done = true;
                }
            }

            Result GetResult()
            {
                return inner_rc;
            }

            operator bool()
            {
                return R_SUCCEEDED(inner_rc);
            }

            template<typename T>
            void Write(T t)
            {
                memcpy(&data_block[data_pos], &t, sizeof(T));
                data_pos += sizeof(T);
            }
    };

    template<DMCommandRWFunction ReadFn>
    class DMCommandReader
    {
        private:
            DMCommandCommonHeader response;
            u8 *data_block;
            size_t data_pos;
            Result inner_rc;
            bool read_done;
            bool fn_wait;

        public:
            DMCommandReader(bool wait = false)
            {
                fn_wait = wait;
                read_done = false;
                data_pos = 0;
                data_block = new u8[BlockSize]();
                inner_rc = ReadFn(&response, sizeof(DMCommandCommonHeader), fn_wait);
                if(R_SUCCEEDED(inner_rc)) inner_rc = ReadFn(data_block, BlockSize, fn_wait);
            }

            ~DMCommandReader()
            {
                FinishRead();
            }

            void FinishRead()
            {
                if(!read_done)
                {
                    if(data_block) delete[] data_block;
                    read_done = true;
                }
            }

            u32 GetValue()
            {
                return response.val;
            }

            Result GetResult()
            {
                return inner_rc;
            }

            operator bool()
            {
                return R_SUCCEEDED(inner_rc);
            }

            template<typename T>
            T Read()
            {
                T t = *(T*)&data_block[data_pos];
                data_pos += sizeof(T);
                return t;
            }
    };

    class DaemonCommandReader : public DMCommandReader<Daemon_MenuReadImpl>
    {
        public:
            DaemonMessage GetMessage()
            {
                return (DaemonMessage)GetValue();
            }
    };

    class DaemonCommandWriter : public DMCommandWriter<Daemon_MenuWriteImpl>
    {
        public:
            DaemonCommandWriter(MenuMessage msg) : DMCommandWriter((u32)msg)
            {
            }
    };

    class DaemonCommandResultReader : public DMCommandReader<Daemon_MenuReadImpl>
    {
        public:
            DaemonCommandResultReader() : DMCommandReader(true)
            {
            }

            Result GetReadResult()
            {
                return GetValue();
            }
    };

    class DaemonCommandResultWriter : public DMCommandWriter<Daemon_MenuWriteImpl>
    {
        public:
            DaemonCommandResultWriter(Result rc) : DMCommandWriter(rc)
            {
            }
    };

    class MenuCommandReader : public DMCommandReader<Menu_DaemonReadImpl>
    {
        public:
            MenuMessage GetMessage()
            {
                return (MenuMessage)GetValue();
            }
    };

    class MenuCommandWriter : public DMCommandWriter<Menu_DaemonWriteImpl>
    {
        public:
            MenuCommandWriter(DaemonMessage msg) : DMCommandWriter((u32)msg)
            {
            }
    };

    class MenuCommandResultReader : public DMCommandReader<Menu_DaemonReadImpl>
    {
        public:
            MenuCommandResultReader() : DMCommandReader(true)
            {
            }

            Result GetReadResult()
            {
                return GetValue();
            }
    };

    class MenuCommandResultWriter : public DMCommandWriter<Menu_DaemonWriteImpl>
    {
        public:
            MenuCommandResultWriter(Result rc) : DMCommandWriter(rc)
            {
            }
    };
}
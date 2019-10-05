#include <am/am_QCommunications.hpp>
#include <util/util_CFW.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_LibraryApplet.hpp>

namespace am
{
    #define Q_AM_LOOP_TRY_WITH(...) { \
        Result rc = 0; \
        do \
        { \
            __VA_ARGS__ \
            svcSleepThread(10'000'000); \
        } while(R_FAILED(rc)); \
        return rc; \
    }

    #define Q_AM_WAIT_WITH(...) { \
        if(wait) { Q_AM_LOOP_TRY_WITH(__VA_ARGS__) } \
        else \
        { \
            Result rc = 0; \
            __VA_ARGS__ \
            return rc; \
        } \
    }
    
    #define Q_AM_WAIT(expr) Q_AM_WAIT_WITH(rc = (expr);)

    Result QDaemon_LaunchQMenu(QMenuStartMode mode)
    {
        return LibraryAppletStart(AppletId_shop, (u32)mode, NULL, 0);
    }

    ResultWith<QMenuStartMode> QMenu_ProcessInput()
    {
        LibAppletArgs in_args;
        auto rc = QMenu_QDaemonReadImpl(&in_args, sizeof(LibAppletArgs), false);
        return MakeResultWith(rc, (QMenuStartMode)in_args.LaVersion);
    }

    Result QDaemon_QMenuWriteImpl(void *data, size_t size, bool wait)
    Q_AM_WAIT(LibraryAppletSend(data, size))
    

    Result QDaemon_QMenuReadImpl(void *data, size_t size, bool wait)
    Q_AM_WAIT(LibraryAppletRead(data, size))

    Result QMenu_QDaemonWriteImpl(void *data, size_t size, bool wait)
    Q_AM_WAIT_WITH(
        AppletStorage st;
        rc = appletCreateStorage(&st, size);
        if(R_SUCCEEDED(rc))
        {
            rc = appletStorageWrite(&st, 0, data, size);
            if(R_SUCCEEDED(rc)) rc = appletPushOutData(&st);
            appletStorageClose(&st);
        }
    )

    Result QMenu_QDaemonReadImpl(void *data, size_t size, bool wait)
    Q_AM_WAIT_WITH(
        AppletStorage st;
        rc = appletPopInData(&st);
        if(R_SUCCEEDED(rc))
        {
            rc = appletStorageRead(&st, 0, data, size);
            appletStorageClose(&st);
        }
    )
}
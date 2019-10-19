#include <am/am_QCommunications.hpp>
#include <util/util_CFW.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_LibraryApplet.hpp>

namespace am
{
    Service daemon_srv;

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
        return LibraryAppletStart(QMenuAppletId, (u32)mode, NULL, 0);
    }

    Result QDaemon_LaunchQHbTarget(hb::TargetInput input)
    {
        return LibraryAppletStart(QHbTargetAppletId, (u32)am::Magic, &input, sizeof(input));
    }

    Result QLibraryAppletReadStorage(void *data, size_t size)
    {
        return QMenu_QDaemonReadImpl(data, size, false);
    }

    Result QApplicationReadStorage(void *data, size_t size)
    {
        AppletStorage st;
        auto rc = appletPopLaunchParameter(&st, AppletLaunchParameterKind_UserChannel);
        if(R_SUCCEEDED(rc))
        {
            rc = appletStorageRead(&st, 0, data, size);
            appletStorageClose(&st);
        }
        return rc;
    }

    ResultWith<QMenuStartMode> QMenu_ProcessInput()
    {
        LibAppletArgs in_args;
        auto rc = QLibraryAppletReadStorage(&in_args, sizeof(LibAppletArgs));
        return MakeResultWith(rc, (QMenuStartMode)in_args.LaVersion);
    }

    Result QMenu_InitializeDaemonService()
    {
        if(serviceIsActive(&daemon_srv)) return 0;
        return smGetService(&daemon_srv, AM_QDAEMON_SERVICE_NAME);
    }

    ResultWith<QMenuMessage> QMenu_GetLatestQMenuMessage()
    {
        QMenuMessage msg = QMenuMessage::Invalid;

        IpcCommand c;
        ipcInitialize(&c);

        struct Raw
        {
            u64 magic;
            u64 cmdid;
        } *raw = (struct Raw*)ipcPrepareHeader(&c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmdid = 0;

        auto rc = serviceIpcDispatch(&daemon_srv);

        if(R_SUCCEEDED(rc))
        {
            IpcParsedCommand r;
            ipcParse(&r);

            struct Response
            {
                u64 magic;
                u64 res;
                u32 msg;
            } *resp = (struct Response*)r.Raw;

            rc = resp->res;

            if(R_SUCCEEDED(rc)) msg = (QMenuMessage)resp->msg;
        }

        return MakeResultWith(rc, msg);
    }

    void QMenu_FinalizeDaemonService()
    {
        serviceClose(&daemon_srv);
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
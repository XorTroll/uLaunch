#include <am/am_DaemonMessages.hpp>

namespace {

    Service g_DaemonPrivateService;

    Result daemonPrivateInitialize(Service *srv) {
        u64 pid_placeholder = 0;
        return serviceDispatchIn(srv, 0, pid_placeholder,
            .in_send_pid = true
        );
    }

    Result daemonPrivateGetMessage(Service *srv, dmi::MenuMessage *out_msg) {
        return serviceDispatchOut(&g_DaemonPrivateService, 1, out_msg);
    }

    Result daemonInitializePrivateService() {
        if(serviceIsActive(&g_DaemonPrivateService)) {
            return ResultSuccess;
        }

        R_TRY(smGetService(&g_DaemonPrivateService, AM_DAEMON_PRIVATE_SERVICE_NAME));
        R_TRY(daemonPrivateInitialize(&g_DaemonPrivateService));

        return ResultSuccess;
    }

    void daemonFinalizePrivateService() {
        serviceClose(&g_DaemonPrivateService);
    }

    dmi::MenuMessage daemonGetMessage() {
        auto msg = dmi::MenuMessage::Invalid;
        UL_ASSERT(daemonPrivateGetMessage(&g_DaemonPrivateService, &msg));
        return msg;
    }

}

namespace am {

    namespace {

        Mutex g_StopLock = EmptyMutex;
        Mutex g_ReceiverLock = EmptyMutex;
        bool g_Initialized = false;
        bool g_ReceiveThreadShouldStop = false;
        Thread g_ReceiverThread;
        std::vector<std::pair<MessageDetectCallback, dmi::MenuMessage>> g_ReceiverCallbackTable;

        void DaemonMessageReceiverThread(void*) {
            while(true) {
                mutexLock(&g_StopLock);
                auto should_stop = g_ReceiveThreadShouldStop;
                mutexUnlock(&g_StopLock);
                if(should_stop) {
                    break;
                }

                auto tmp_msg = daemonGetMessage();

                mutexLock(&g_ReceiverLock);
                for(auto &[cb, msg] : g_ReceiverCallbackTable) {
                    if(msg == tmp_msg) {
                        cb();
                    }
                }
                mutexUnlock(&g_ReceiverLock);

                svcSleepThread(10'000'000ul);
            }
        }

    }

    Result InitializeDaemonMessageHandler() {
        if(g_Initialized) {
            return ResultSuccess;
        }
        R_TRY(daemonInitializePrivateService());

        g_ReceiveThreadShouldStop = false;
        R_TRY(threadCreate(&g_ReceiverThread, &DaemonMessageReceiverThread, nullptr, nullptr, 0x1000, 0x2b, -2));
        R_TRY(threadStart(&g_ReceiverThread));

        g_Initialized = true;
        return ResultSuccess;
    }

    void ExitDaemonMessageHandler() {
        if(!g_Initialized) {
            return;
        }
        
        mutexLock(&g_StopLock);
        g_ReceiveThreadShouldStop = true;
        mutexUnlock(&g_StopLock);

        threadWaitForExit(&g_ReceiverThread);
        threadClose(&g_ReceiverThread);

        daemonFinalizePrivateService();
        g_Initialized = false;
    }

    void RegisterOnMessageDetect(MessageDetectCallback callback, dmi::MenuMessage desired_msg) {
        mutexLock(&g_ReceiverLock);
        g_ReceiverCallbackTable.push_back(std::make_pair(callback, desired_msg));
        mutexUnlock(&g_ReceiverLock);
    }

}
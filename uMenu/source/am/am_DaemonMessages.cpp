#include <am/am_DaemonMessages.hpp>

namespace am {

    namespace {

        Mutex g_ReceiverLock = EmptyMutex;
        Service g_DaemonPrivateService;
        bool g_Initialized = false;
        bool g_ReceiveThreadShouldStop = false;
        Thread g_ReceiverThread;
        std::vector<std::pair<MessageDetectCallback, dmi::MenuMessage>> g_ReceiverCallbackTable;

        void DaemonMessageReceiverThread(void*) {
            while(true) {
                mutexLock(&g_ReceiverLock);
                auto should_stop = g_ReceiveThreadShouldStop;
                mutexUnlock(&g_ReceiverLock);
                if(should_stop) {
                    break;
                }

                auto tmp_msg = dmi::MenuMessage::Invalid;
                u64 pid_placeholder = 0;
                UL_ASSERT(serviceDispatchInOut(&g_DaemonPrivateService, 0, pid_placeholder, tmp_msg,
                    .in_send_pid = true,
                ));

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
        R_TRY(smGetService(&g_DaemonPrivateService, AM_DAEMON_PRIVATE_SERVICE_NAME));

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
        
        mutexLock(&g_ReceiverLock);
        g_ReceiveThreadShouldStop = true;
        mutexUnlock(&g_ReceiverLock);

        threadWaitForExit(&g_ReceiverThread);
        threadClose(&g_ReceiverThread);

        serviceClose(&g_DaemonPrivateService);
        g_Initialized = false;
    }

    void RegisterOnMessageDetect(MessageDetectCallback callback, dmi::MenuMessage desired_msg) {
        mutexLock(&g_ReceiverLock);
        g_ReceiverCallbackTable.push_back(std::make_pair(callback, desired_msg));
        mutexUnlock(&g_ReceiverLock);
    }

}